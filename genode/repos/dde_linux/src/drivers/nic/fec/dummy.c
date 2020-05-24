#include <lx_emul.h>
#include <linux/phy.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/skbuff.h>
#include <linux/timecounter.h>
#include <net/tso.h>

void bus_unregister(struct bus_type *bus)
{
	TRACE_AND_STOP;
}

void class_unregister(struct class *cls)
{
	TRACE_AND_STOP;
}

size_t copy_from_user(void *to, void const *from, size_t len)
{
	TRACE_AND_STOP;
	return -1;
}

size_t copy_to_user(void *dst, void const *src, size_t len)
{
	TRACE_AND_STOP;
	return -1;
}

bool device_may_wakeup(struct device *dev)
{
	TRACE_AND_STOP;
	return -1;
}

void device_release_driver(struct device *dev)
{
	TRACE_AND_STOP;
}

int disable_irq(unsigned int irq)
{
	TRACE_AND_STOP;
	return -1;
}

int disable_irq_wake(unsigned int irq)
{
	TRACE_AND_STOP;
	return -1;
}

void dma_free_coherent(struct device * d, size_t size, void *vaddr, dma_addr_t bus)
{
	TRACE_AND_STOP;
}

void driver_unregister(struct device_driver *drv)
{
	TRACE_AND_STOP;
}

void dst_release(struct dst_entry *dst)
{
	TRACE_AND_STOP;
}

int enable_irq_wake(unsigned int irq)
{
	TRACE_AND_STOP;
	return -1;
}

void eth_hw_addr_random(struct net_device *dev)
{
	TRACE_AND_STOP;
}

void ethtool_convert_legacy_u32_to_link_mode(unsigned long *dst, u32 legacy_u32)
{
	TRACE_AND_STOP;
}

bool ethtool_convert_link_mode_to_legacy_u32(u32 *legacy_u32,
                                             const unsigned long *src)
{
	TRACE_AND_STOP;
}

u32 ethtool_op_get_link(struct net_device * d)
{
	TRACE_AND_STOP;
	return -1;
}

int ethtool_op_get_ts_info(struct net_device * d, struct ethtool_ts_info * i)
{
	TRACE_AND_STOP;
	return -1;
}

bool file_ns_capable(const struct file *file, struct user_namespace *ns, int cap)
{
	TRACE_AND_STOP;
	return -1;
}

void free_irq(unsigned int irq, void *dev_id)
{
	TRACE_AND_STOP;
}

void free_netdev(struct net_device * d)
{
	TRACE_AND_STOP;
}

void free_uid(struct user_struct *dummy)
{
	TRACE_AND_STOP;
}

bool gfp_pfmemalloc_allowed(gfp_t g)
{
	TRACE_AND_STOP;
	return -1;
}

bool gfpflags_allow_blocking(const gfp_t gfp_flags)
{
	TRACE_AND_STOP;
}

int in_irq(void)
{
	TRACE_AND_STOP;
	return -1;
}

struct iphdr *ip_hdr(const struct sk_buff *skb)
{
	TRACE_AND_STOP;
	return NULL;
}

void *kmap_atomic(struct page *page)
{
	TRACE_AND_STOP;
	return NULL;
}

const char *kobject_name(const struct kobject *kobj)
{
	TRACE_AND_STOP;
}

void kunmap_atomic(void *addr)
{
	TRACE_AND_STOP;
}

void might_sleep()
{
	TRACE_AND_STOP;
}

void mm_unaccount_pinned_pages(struct mmpin *mmp)
{
	TRACE_AND_STOP;
}

void module_put(struct module *mod)
{
	TRACE_AND_STOP;
}

void netif_tx_disable(struct net_device *dev)
{
	TRACE_AND_STOP;
}

void netif_tx_stop_queue(struct netdev_queue *dev_queue)
{
	TRACE_AND_STOP;
}

void netif_tx_wake_all_queues(struct net_device * d)
{
	TRACE_AND_STOP;
}

void netif_tx_wake_queue(struct netdev_queue *dev_queue)
{
	TRACE_AND_STOP;
}

int net_ratelimit(void)
{
	TRACE_AND_STOP;
	return -1;
}

ktime_t ns_to_ktime(u64 ns)
{
	ktime_t ret;
	TRACE_AND_STOP;
	return ret;
}

struct timespec64 ns_to_timespec64(const s64 nsec)
{
	struct timespec64 ret;
	TRACE_AND_STOP;
	return ret;
}

int of_device_uevent_modalias(struct device *dev, struct kobj_uevent_env *env)
{
	TRACE_AND_STOP;
}

int of_mdio_parse_addr(struct device *dev, const struct device_node *np)
{
	TRACE_AND_STOP;
}

struct device_node *of_node_get(struct device_node *node)
{
	TRACE_AND_STOP;
	return NULL;
}

void of_phy_deregister_fixed_link(struct device_node *np)
{
	TRACE_AND_STOP;
}

int of_phy_register_fixed_link(struct device_node *np)
{
	TRACE_AND_STOP;
	return -1;
}

bool of_property_read_bool(const struct device_node *np, const char *propname)
{
	TRACE_AND_STOP;
}

void phy_led_triggers_unregister(struct phy_device *phy)
{
	TRACE_AND_STOP;
}

void pm_runtime_disable(struct device *dev)
{
	TRACE_AND_STOP;
}

int pm_runtime_put(struct device *dev)
{
	TRACE_AND_STOP;
}

void ptp_clock_event(struct ptp_clock *ptp, struct ptp_clock_event *event)
{
	TRACE_AND_STOP;
}

int ptp_clock_index(struct ptp_clock *ptp)
{
	TRACE_AND_STOP;
	return -1;
}

int ptp_clock_unregister(struct ptp_clock *ptp)
{
	TRACE_AND_STOP;
	return -1;
}

bool page_is_pfmemalloc(struct page *page)
{
	TRACE_AND_STOP;
	return -1;
}

void put_page(struct page *page)
{
	TRACE_AND_STOP;
}

void read_lock_bh(rwlock_t * l)
{
	TRACE_AND_STOP;
}

void read_unlock_bh(rwlock_t * l)
{
	TRACE_AND_STOP;
}

int regulator_disable(struct regulator *r)
{
	TRACE_AND_STOP;
	return -1;
}

int request_threaded_irq(unsigned int irq, irq_handler_t handler,
                         irq_handler_t thread_fn,
                         unsigned long flags, const char *name, void *dev)
{
	TRACE_AND_STOP;
}

void sk_free(struct sock *sk)
{
	TRACE_AND_STOP;
}

void sock_efree(struct sk_buff *skb)
{
	TRACE_AND_STOP;
}

int strcmp(const char *s1, const char *s2)
{
	TRACE_AND_STOP;
	return -1;
}

char *strncpy(char *dst, const char *src, size_t s)
{
	TRACE_AND_STOP;
}

int sysfs_create_link_nowarn(struct kobject *kobj,
                             struct kobject *target,
                             const char *name)
{
	TRACE_AND_STOP;
}

void sysfs_remove_link(struct kobject *kobj, const char *name)
{
	TRACE_AND_STOP;
}

unsigned int tcp_hdrlen(const struct sk_buff *skb)
{
	TRACE_AND_STOP;
	return -1;
}

u64 timecounter_cyc2time(struct timecounter *tc, cycle_t cycle_tstamp)
{
	TRACE_AND_STOP;
	return -1;
}

void tso_build_data(struct sk_buff *skb, struct tso_t *tso, int size)
{
	TRACE_AND_STOP;
}

void tso_build_hdr(struct sk_buff *skb, char *hdr, struct tso_t *tso, int size, bool is_last)
{
	TRACE_AND_STOP;
}

int tso_count_descs(struct sk_buff *skb)
{
	TRACE_AND_STOP;
	return -1;
}

void tso_start(struct sk_buff *skb, struct tso_t *tso)
{
	TRACE_AND_STOP;
}

void unregister_netdev(struct net_device * d)
{
	TRACE_AND_STOP;
}

void usleep_range(unsigned long min, unsigned long max)
{
	TRACE_AND_STOP;
}

void __vlan_hwaccel_put_tag(struct sk_buff *skb, __be16 vlan_proto, u16 vlan_tci)
{
	TRACE_AND_STOP;
}

struct user_namespace init_user_ns;
int sysctl_tstamp_allow_data;
