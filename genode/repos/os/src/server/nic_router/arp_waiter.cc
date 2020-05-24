/*
 * \brief  Aspect of waiting for an ARP reply
 * \author Martin Stein
 * \date   2016-08-19
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* local includes */
#include <arp_waiter.h>
#include <interface.h>
#include <domain.h>

using namespace Net;
using namespace Genode;


Arp_waiter::Arp_waiter(Interface               &src,
                       Domain                  &dst,
                       Ipv4_address      const &ip,
                       Packet_descriptor const &packet)
:
	_src_le(this), _src(src), _dst_le(this), _dst(dst), _ip(ip),
	_packet(packet)
{
	_src.arp_stats().alive++;
	_src.own_arp_waiters().insert(&_src_le);
	_dst().foreign_arp_waiters().insert(&_dst_le);
}


Arp_waiter::~Arp_waiter()
{
	_src.arp_stats().alive--;
	_src.arp_stats().destroyed++;
	_src.own_arp_waiters().remove(&_src_le);
	_dst().foreign_arp_waiters().remove(&_dst_le);
}


void Arp_waiter::handle_config(Domain &dst)
{
	_dst().foreign_arp_waiters().remove(&_dst_le);
	_dst = dst;
	_dst().foreign_arp_waiters().insert(&_dst_le);
}


void Arp_waiter::print(Output &output) const
{
	Genode::print(output, "IP ", _ip, " DST ", _dst());
}
