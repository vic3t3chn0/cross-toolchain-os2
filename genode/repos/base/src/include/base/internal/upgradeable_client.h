/*
 * \brief  Utility for using a dynamically upgradeable session
 * \author Norman Feske
 * \date   2013-09-25
 */

/*
 * Copyright (C) 2006-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__INTERNAL__UPGRADEABLE_CLIENT_H_
#define _INCLUDE__BASE__INTERNAL__UPGRADEABLE_CLIENT_H_

#include <base/env.h>
#include <base/log.h>

namespace Genode { template <typename> struct Upgradeable_client; }


/**
 * Client object for a session that may get its session quota upgraded
 */
template <typename CLIENT>
struct Genode::Upgradeable_client : CLIENT
{
	typedef Genode::Capability<typename CLIENT::Rpc_interface> Capability;

	Parent &_parent;
	Parent::Client::Id _id;

	Upgradeable_client(Parent &parent, Capability cap, Parent::Client::Id id)
	: CLIENT(cap), _parent(parent), _id(id) { }

	void upgrade_ram(size_t quota)
	{
		_parent.upgrade(_id, String<64>("ram_quota=", quota).string());
	}

	void upgrade_caps(size_t quota)
	{
		_parent.upgrade(_id, String<64>("cap_quota=", quota).string());
	}
};

#endif /* _INCLUDE__BASE__INTERNAL__UPGRADEABLE_CLIENT_H_ */
