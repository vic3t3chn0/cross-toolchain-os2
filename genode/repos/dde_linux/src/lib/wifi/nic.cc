/*
 * \brief  Glue code for Linux network drivers
 * \author Sebastian Sumpf
 * \author Josef Soentgen
 * \date   2012-07-05
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/log.h>
#include <base/rpc_server.h>
#include <base/snprintf.h>
#include <base/tslab.h>
#include <nic/xml_node.h>
#include <nic/component.h>
#include <root/component.h>
#include <util/xml_node.h>

/* local includes */
#include <lx.h>
#include <lx_emul.h>

#include <lx_kit/env.h>

#include <lx_emul/extern_c_begin.h>
# include <linux/skbuff.h>
# include <net/cfg80211.h>
# include <lxc.h>
#include <lx_emul/extern_c_end.h>


enum {
	HEAD_ROOM = 128, /* XXX guessed value but works */
};


struct Tx_data
{
	net_device     *ndev;
	struct sk_buff *skb;
	Lx::Task       *task;
	int             err;
};

static Lx::Task *_tx_task;
static Tx_data   _tx_data;


static void _run_tx_task(void *args)
{
	Tx_data *data = static_cast<Tx_data*>(args);

	while (1) {
		Lx::scheduler().current()->block_and_schedule();

		net_device    *ndev = data->ndev;
		struct sk_buff *skb = data->skb;

		data->err = ndev->netdev_ops->ndo_start_xmit(skb, ndev);
		if (data->err) {
			Genode::warning("xmit failed: ", data->err, " skb: ", skb);
		}

		data->skb = nullptr;

		if (data->task) {
			data->task->unblock();
			data->task = nullptr;
		}
	}
}


bool tx_task_send(struct sk_buff *skb)
{
	if (_tx_data.skb) {
		Genode::error("skb: ", skb, " already queued");
		return false;
	}

	if (!_tx_task) {
		Genode::error("no TX task available");
		return false;
	}

	_tx_data.ndev = skb->dev;
	_tx_data.skb  = skb;
	_tx_data.task = Lx::scheduler().current();

	_tx_task->unblock();
	Lx::scheduler().current()->block_and_schedule();

	return true;
}


/**
 * Nic::Session implementation
 */
class Wifi_session_component : public Nic::Session_component
{
	private:

		net_device *_ndev;
		bool        _has_link = !(_ndev->state & 1UL << __LINK_STATE_NOCARRIER);

	protected:

		bool _send()
		{
			using namespace Genode;

			/*
			 * We must not be called from another task, just from the
			 * packet stream dispatcher.
			 */
			if (Lx::scheduler().active()) {
				warning("scheduler active");
				return false;
			}

			/* XXX enable as soon as the other stack plays along
			if (!_has_link) {
				Packet_descriptor packet = _tx.sink()->get_packet();
				_tx.sink()->acknowledge_packet(packet);
				Genode::warning("no link, drop packet");
				return true;
			}
			*/

			if (!_tx.sink()->ready_to_ack()) { return false; }
			if (!_tx.sink()->packet_avail()) { return false; }

			Packet_descriptor packet = _tx.sink()->get_packet();
			if (!packet.size() || !_tx.sink()->packet_valid(packet)) {
				warning("invalid tx packet");
				return true;
			}

			struct sk_buff *skb = lxc_alloc_skb(packet.size() + HEAD_ROOM, HEAD_ROOM);

			skb->dev = _ndev;

			unsigned char *data = lxc_skb_put(skb, packet.size());
			Genode::memcpy(data, _tx.sink()->packet_content(packet), packet.size());

			_tx_data.ndev = _ndev;
			_tx_data.skb  = skb;

			_tx_task->unblock();
			Lx::scheduler().schedule();

			_tx.sink()->acknowledge_packet(packet);

			return true;
		}

		void _handle_rx()
		{
			while (_rx.source()->ack_avail()) {
				_rx.source()->release_packet(_rx.source()->get_acked_packet());
			}
		}

		void _handle_packet_stream() override
		{
			_handle_rx();

			while (_send()) { continue; }
		}

	public:

		Wifi_session_component(Genode::size_t const tx_buf_size,
		                       Genode::size_t const rx_buf_size,
		                       Genode::Allocator   &rx_block_md_alloc,
		                       Genode::Env  &env, net_device *ndev)
		: Session_component(tx_buf_size, rx_buf_size, Genode::CACHED,
		                    rx_block_md_alloc, env),
		  _ndev(ndev)
		{
			_ndev->lx_nic_device = this;
		}

		~Wifi_session_component()
		{
			_ndev->lx_nic_device = nullptr;
		}

		/**
		 * Report link state
		 */
		void link_state(bool link)
		{
			/* only report changes of the link state */
			if (link == _has_link)
				return;

			_has_link = link;
			_link_state_changed();
		}

		void receive(struct sk_buff *skb)
		{
			_handle_rx();

			if (!_rx.source()->ready_to_submit()) {
				Genode::warning("not ready to receive packet");
				return;
			}

			Skb s = skb_helper(skb);

			try {
				Nic::Packet_descriptor p = _rx.source()->alloc_packet(s.packet_size + s.frag_size);
				void *buffer = _rx.source()->packet_content(p);
				memcpy(buffer, s.packet, s.packet_size);

				if (s.frag_size)
					memcpy((char *)buffer + s.packet_size, s.frag, s.frag_size);

				_rx.source()->submit_packet(p);
			} catch (...) {
				Genode::warning("failed to process received packet");
			}
		}

		/*****************************
		 ** NIC-component interface **
		 *****************************/

		Nic::Mac_address mac_address() override
		{
			Nic::Mac_address m;
			Genode::memcpy(&m, _ndev->perm_addr, ETH_ALEN);
			return m;
		}

		bool link_state() override { return _has_link; }
};


/**
 * NIC root implementation
 */
class Root : public Genode::Root_component<Wifi_session_component,
                                           Genode::Single_client>
{
	private:

		Genode::Env &_env;

	protected:

		Wifi_session_component *_create_session(const char *args)
		{
			using namespace Genode;
			using Genode::size_t;

			size_t ram_quota   = Arg_string::find_arg(args, "ram_quota"  ).ulong_value(0);
			size_t tx_buf_size = Arg_string::find_arg(args, "tx_buf_size").ulong_value(0);
			size_t rx_buf_size = Arg_string::find_arg(args, "rx_buf_size").ulong_value(0);

			/* deplete ram quota by the memory needed for the session structure */
			size_t session_size = max(4096UL, (unsigned long)sizeof(Wifi_session_component));
			if (ram_quota < session_size)
				throw Genode::Insufficient_ram_quota();

			/*
			 * Check if donated ram quota suffices for both communication
			 * buffers and check for overflow
			 */
			if (tx_buf_size + rx_buf_size < tx_buf_size ||
			    tx_buf_size + rx_buf_size > ram_quota - session_size) {
				Genode::error("insufficient 'ram_quota', got ", ram_quota, " need ",
				              tx_buf_size + rx_buf_size + session_size);
				throw Genode::Insufficient_ram_quota();
			}

			session = new (md_alloc())
			          Wifi_session_component(tx_buf_size, rx_buf_size,
			                                *md_alloc(), _env, device);
			return session;
		}

		void _destroy_session(Wifi_session_component *session)
		{
			/* stop rx */
			Root::instance->session = nullptr;
			Genode::Root_component<Wifi_session_component, Genode::Single_client>::_destroy_session(session);
		}

	public:

		net_device             *device  = nullptr;
		Wifi_session_component *session = nullptr;
		static Root            *instance;

		Root(Genode::Env &env, Genode::Allocator &md_alloc)
		: Genode::Root_component<Wifi_session_component, Genode::Single_client>(&env.ep().rpc_ep(), &md_alloc),
			_env(env)
		{ }

		void announce() { _env.parent().announce(_env.ep().manage(*this)); }
};


Root *Root::instance;


void Lx::nic_init(Genode::Env &env, Genode::Allocator &alloc)
{
	static Root root(env, alloc);
	Root::instance = &root;
}


void Lx::get_mac_address(unsigned char *addr)
{
	memcpy(addr, Root::instance->device->perm_addr, ETH_ALEN);
}


namespace Lx {
	class Notifier;
}


class Lx::Notifier
{
	private:

		struct Block : public Genode::List<Block>::Element
		{
			struct notifier_block *nb;

			Block(struct notifier_block *nb) : nb(nb) { }
		};

		Lx_kit::List<Block> _list;
		Genode::Tslab<Block, 32 * sizeof(Block)> _block_alloc;

		void *_ptr;

	public:

		Notifier(Genode::Allocator &alloc, void *ptr)
		: _block_alloc(&alloc), _ptr(ptr) { }

		virtual ~Notifier() { };

		bool handles(void *ptr)
		{
			return _ptr == ptr;
		}

		void register_block(struct notifier_block *nb)
		{
			Block *b = new (&_block_alloc) Block(nb);
			_list.insert(b);
		}

		void unregister_block(struct notifier_block *nb)
		{
			for (Block *b = _list.first(); b; b = b->next())
				if (b->nb == nb) {
					_list.remove(b);
					destroy(&_block_alloc, b);
					break;
				}
		}

		int call_all_blocks(unsigned long val, void *v)
		{
			int rv = NOTIFY_DONE;
			for (Block *b = _list.first(); b; b = b->next()) {
				rv = b->nb->notifier_call(b->nb, val, v);
				if (rv & NOTIFY_STOP_MASK)
					break;
			}
			return rv;
		}
};


static Genode::Registry<Genode::Registered<Lx::Notifier>> _blocking_notifier_registry;


/* XXX move blocking_notifier_call to proper location */
/**********************
 ** linux/notifier.h **
 **********************/

static Lx::Notifier &blocking_notifier(struct blocking_notifier_head *nh)
{
	Lx::Notifier *notifier = nullptr;
	auto lookup = [&](Lx::Notifier &n) {
		if (!n.handles(nh)) { return; }

		notifier = &n;
	};
	_blocking_notifier_registry.for_each(lookup);

	if (!notifier) {
		Genode::Registered<Lx::Notifier> *n = new (&Lx_kit::env().heap())
			Genode::Registered<Lx::Notifier>(_blocking_notifier_registry,
			                                 Lx_kit::env().heap(), nh);
		notifier = &*n;
	}

	return *notifier;
}


int blocking_notifier_chain_register(struct blocking_notifier_head *nh,
                                          struct notifier_block *nb)
{
	blocking_notifier(nh).register_block(nb);
	return 0;
}

int blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
                                          struct notifier_block *nb)
{
	blocking_notifier(nh).unregister_block(nb);
	return 0;
}


int blocking_notifier_call_chain(struct blocking_notifier_head *nh,
                                 unsigned long val, void *v)
{
	return blocking_notifier(nh).call_all_blocks(val, v);
}


/***********************
 ** linux/netdevice.h **
 ***********************/

static Lx::Notifier &net_notifier()
{
	static Lx::Notifier inst(Lx_kit::env().heap(), NULL);
	return inst;
}


extern "C" int register_netdevice_notifier(struct notifier_block *nb)
{
	/**
	 * In Linux the nb is actually called upon on registration. We do not
	 * that semantic because we add a net_device only after all notifiers
	 * were registered.
	 */
	net_notifier().register_block(nb);

	return 0;
}


extern "C" int unregster_netdevice_notifier(struct notifier_block *nb)
{
	net_notifier().unregister_block(nb);

	return 0;
}


extern "C" struct net_device * netdev_notifier_info_to_dev(struct netdev_notifier_info *info)
{
	/* we always pass a net_device pointer to this function */
	return reinterpret_cast<net_device *>(info);
}


struct Proto_hook : public Lx_kit::List<Proto_hook>::Element
{
	struct packet_type &pt;

	Proto_hook(struct packet_type *pt) : pt(*pt) { }
};


class Proto_hook_list
{
	private:

		Lx_kit::List<Proto_hook>  _list;
		Genode::Allocator    &_alloc;

	public:

		Proto_hook_list(Genode::Allocator &alloc) : _alloc(alloc) { }

		void insert(struct packet_type *pt) {
			_list.insert(new (&_alloc) Proto_hook(pt)); }

		void remove(struct packet_type *pt)
		{
			for (Proto_hook *ph = _list.first(); ph; ph = ph->next())
				if (&ph->pt == pt) {
					_list.remove(ph);
					destroy(&_alloc, ph);
					break;
				}
		}

		Proto_hook* first() { return _list.first(); }
};


static Proto_hook_list& proto_hook_list()
{
	static Proto_hook_list inst(Lx_kit::env().heap());
	return inst;
}


extern "C" void dev_add_pack(struct packet_type *pt)
{
	proto_hook_list().insert(pt);
}


extern "C" void __dev_remove_pack(struct packet_type *pt)
{
	proto_hook_list().remove(pt);
}


extern "C" struct net_device *__dev_get_by_index(struct net *net, int ifindex)
{
	if (!Root::instance->device) {
		Genode::error("no net device registered!");
		return 0;
	}

	return Root::instance->device;
}


extern "C" struct net_device *dev_get_by_index_rcu(struct net *net, int ifindex)
{
	return __dev_get_by_index(net, ifindex);
}


extern "C" struct net_device *dev_get_by_index(struct net *net, int ifindex)
{
	return __dev_get_by_index(net, ifindex);
}


extern "C" int dev_hard_header(struct sk_buff *skb, struct net_device *dev,
                               unsigned short type, const void *daddr,
                               const void *saddr, unsigned int len)
{
	if (!dev->header_ops || !dev->header_ops->create)
		return 0;

	return dev->header_ops->create(skb, dev, type, daddr, saddr, len);
}


extern "C" int dev_parse_header(const struct sk_buff *skb, unsigned char *haddr)
{
	struct net_device const *dev = skb->dev;

	if (!dev->header_ops || dev->header_ops->parse)
		return 0;

	return dev->header_ops->parse(skb, haddr);
}


extern "C" int dev_queue_xmit(struct sk_buff *skb)
{
	struct net_device *dev           = skb->dev;
	struct net_device_ops const *ops = dev->netdev_ops;

	if (skb->next) {
		Genode::warning("more skb's queued");
	}

	return tx_task_send(skb) ? NETDEV_TX_OK : -1;
}


extern "C" size_t LL_RESERVED_SPACE(struct net_device *dev)
{
	return dev->hard_header_len ?
	       (dev->hard_header_len + (HH_DATA_MOD - 1)) & ~(HH_DATA_MOD - 1) : 0;
}


extern "C" void dev_close(struct net_device *ndev)
{
	/*
	 * First instruct cfg80211 to leave the associated network
	 * and then shutdown the interface.
	 */
	net_notifier().call_all_blocks(NETDEV_GOING_DOWN, ndev);
	net_notifier().call_all_blocks(NETDEV_DOWN, ndev);

	ndev->state &= ~(1UL << __LINK_STATE_START);
	netif_carrier_off(ndev);

	const struct net_device_ops *ops = ndev->netdev_ops;
	if (ops->ndo_stop) { ops->ndo_stop(ndev); }

	ndev->flags &= ~IFF_UP;
}


bool Lx::open_device()
{
	if (!Root::instance->device) {
		Genode::error("no net_device available");
		return false;
	}

	struct net_device * const ndev = Root::instance->device;

	int err = ndev->netdev_ops->ndo_open(ndev);
	if (err) {
		Genode::error("Open device failed");
		throw -1;
		return err;
	}

	/*
	 * Important, otherwise netif_running checks fail and AF_PACKET
	 * will not bind and EAPOL will cease to work.
	 */
	ndev->flags |= IFF_UP;
	ndev->state |= (1UL << __LINK_STATE_START);

	if (ndev->netdev_ops->ndo_set_rx_mode)
		ndev->netdev_ops->ndo_set_rx_mode(ndev);

	net_notifier().call_all_blocks(NETDEV_UP, ndev);

	return true;
}


extern "C" int register_netdevice(struct net_device *ndev)
{
	static bool already_registered = false;

	if (already_registered) {
		Genode::error("We don't support multiple network devices in one driver instance");
		return -ENODEV;
	}

	already_registered = true;

	Root::instance->device = ndev;

	ndev->state |= 1UL << __LINK_STATE_START;
	netif_carrier_off(ndev);

	/* execute all notifier blocks */
	net_notifier().call_all_blocks(NETDEV_REGISTER, ndev);
	net_notifier().call_all_blocks(NETDEV_UP, ndev);
	ndev->ifindex = 1;

	/* set mac adress */
	Genode::memcpy(ndev->perm_addr, ndev->ieee80211_ptr->wiphy->perm_addr, ETH_ALEN);

	{
		using namespace Genode;
		log("mac_address: ",
		    Hex(ndev->perm_addr[0], Hex::OMIT_PREFIX, Hex::PAD), ":",
		    Hex(ndev->perm_addr[1], Hex::OMIT_PREFIX, Hex::PAD), ":",
		    Hex(ndev->perm_addr[2], Hex::OMIT_PREFIX, Hex::PAD), ":",
		    Hex(ndev->perm_addr[3], Hex::OMIT_PREFIX, Hex::PAD), ":",
		    Hex(ndev->perm_addr[4], Hex::OMIT_PREFIX, Hex::PAD), ":",
		    Hex(ndev->perm_addr[5], Hex::OMIT_PREFIX, Hex::PAD));
	}

	int err = ndev->netdev_ops->ndo_open(ndev);
	if (err) {
		Genode::error("Initializing device failed");
		throw -1;
		return err;
	}

	static Lx::Task tx_task { _run_tx_task, &_tx_data, "tx_task",
	                          Lx::Task::PRIORITY_1, Lx::scheduler() };
	_tx_task = &tx_task;

	if (ndev->netdev_ops->ndo_set_rx_mode)
		ndev->netdev_ops->ndo_set_rx_mode(ndev);

	Root::instance->announce();

	list_add_tail_rcu(&ndev->dev_list, &init_net.dev_base_head);

	return 0;
}


extern "C" int netif_running(const struct net_device *dev)
{
	return dev->state & (1UL << __LINK_STATE_START);
}


extern "C" int netif_device_present(struct net_device *dev) { return 1; }


extern "C" int netif_carrier_ok(const struct net_device *dev)
{
	return !(dev->state & (1UL << __LINK_STATE_NOCARRIER));
}


extern "C" void netif_carrier_on(struct net_device *dev)
{
	dev->state &= ~(1UL << __LINK_STATE_NOCARRIER);

	Wifi_session_component *session = (Wifi_session_component *)dev->lx_nic_device;

	if (session)
		session->link_state(true);
}


extern "C" void netif_carrier_off(struct net_device *dev)
{
	dev->state |= 1UL << __LINK_STATE_NOCARRIER;

	Wifi_session_component *session = (Wifi_session_component *)dev->lx_nic_device;

	if (session)
		session->link_state(false);
}


extern "C" int netif_receive_skb(struct sk_buff *skb)
{
	/**
	 * XXX check original linux implementation if it is really
	 * necessary to free the skb if it was not handled.
	 */

	/* send EAPOL related frames only to the wpa_supplicant */
	if (is_eapol(skb)) {
		/* XXX call only AF_PACKET hook */
		for (Proto_hook* ph = proto_hook_list().first(); ph; ph = ph->next()) {
			ph->pt.func(skb, Root::instance->device, &ph->pt, Root::instance->device);
		}
		return NET_RX_SUCCESS;
	}

	if (Root::instance->session)
		Root::instance->session->receive(skb);

	dev_kfree_skb(skb);
	return NET_RX_SUCCESS;
}


gro_result_t napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	return netif_receive_skb(skb);
}


extern "C" void netif_start_subqueue(struct net_device *dev, u16 queue_index)
{
	dev->_tx[queue_index].state = NETDEV_QUEUE_START;
}


extern "C" void netif_stop_subqueue(struct net_device *dev, u16 queue_index)
{
	dev->_tx[queue_index].state = 0;
}


extern "C" void netif_wake_subqueue(struct net_device *dev, u16 queue_index)
{
	dev->_tx[queue_index].state = NETDEV_QUEUE_START;
}


extern "C" u16 netdev_cap_txqueue(struct net_device *dev, u16 queue_index)
{
	if (queue_index > dev-> real_num_tx_queues) {
		Genode::error("queue_index ", queue_index, " out of range (",
		              dev->real_num_tx_queues, " max)");
		return 0;
	}

	return queue_index;
}


extern "C" struct net_device *alloc_netdev_mqs(int sizeof_priv, const char *name,
                                    unsigned char name_assign_type,
                                    void (*setup)(struct net_device *),
                                    unsigned int txqs, unsigned int rxqs)
{
	struct net_device *dev;
	size_t alloc_size;
	struct net_device *p;

	alloc_size = ALIGN(sizeof(struct net_device), NETDEV_ALIGN);
	/* ensure 32-byte alignment of whole construct */
	alloc_size += NETDEV_ALIGN - 1;

	p = (net_device *)kzalloc(alloc_size, GFP_KERNEL);
	if (!p)
		return NULL;

	dev = PTR_ALIGN(p, NETDEV_ALIGN);

	dev->gso_max_size = GSO_MAX_SIZE;
	dev->gso_max_segs = GSO_MAX_SEGS;

	setup(dev);

	/* actually set by dev_open() */
	dev->flags |= IFF_UP;


	/* XXX our dev is always called wlan0 */
	strcpy(dev->name, "wlan0");

	dev->dev_addr = (unsigned char *)kzalloc(ETH_ALEN, GFP_KERNEL);
	if (!dev->dev_addr)
		return 0;

	if (sizeof_priv) {
		/* ensure 32-byte alignment of private area */
		dev->priv = kzalloc(sizeof_priv, GFP_KERNEL);
		if (!dev->priv)
			return 0;
	}

	dev->num_tx_queues = txqs;
	dev->real_num_tx_queues = txqs;
	struct netdev_queue *tx = (struct netdev_queue*)
	                           kcalloc(txqs, sizeof(struct netdev_queue),
	                                   GFP_KERNEL | GFP_LX_DMA);
	if (!tx) {
		Genode::error("could not allocate ndev_queues");
	}

	dev->_tx = tx;
	for (unsigned i = 0; i < txqs; i++) {
		tx[i].dev = dev;
		tx[i].numa_node = NUMA_NO_NODE;
	}

	return dev;
}


/*************************
 ** linux/etherdevice.h **
 *************************/

int is_valid_ether_addr(const u8 *addr)
{
	/* is multicast */
	if ((addr[0] & 0x1))
		return 0;

	/* zero */
	if (!(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]))
		return 0;

	return 1;
}
