/*
 * \brief  MMIO framework utility
 * \author Martin Stein
 * \date   2013-05-17
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__OS__ATTACHED_MMIO_H_
#define _INCLUDE__OS__ATTACHED_MMIO_H_

/* Genode includes */
#include <base/attached_io_mem_dataspace.h>
#include <util/mmio.h>

namespace Genode { class Attached_mmio; }


/**
 * Eases the application of the MMIO framework
 *
 * In the classical case the user wants device memory to be structured
 * by inheriting from Genode::Mmio and using its subclasses. As
 * prerequisite one needs to alloc the IO dataspace, attach it locally
 * and cast the received address. This helper undertakes all of this
 * generic work when inheriting from it.
 */
class Genode::Attached_mmio : public Attached_io_mem_dataspace,
                              public Mmio
{
	public:

		/**
		 * Constructor
		 *
		 * \param base            base address of memory-mapped I/O resource
		 * \param size            size of resource
		 * \param write_combined  enable write combining for the resource
		 *
		 * \throw Service_denied
		 * \throw Insufficient_ram_quota
		 * \throw Insufficient_cap_quota
		 * \throw Out_of_ram
		 * \throw Out_of_caps
		 * \throw Region_map::Region_conflict
		 */
		Attached_mmio(Env &env, addr_t base, size_t size,
		              bool write_combined = false)
		: Attached_io_mem_dataspace(env, base, size, write_combined),
		  Mmio((addr_t)local_addr<void>()) { }
};

#endif /* _INCLUDE__OS__ATTACHED_MMIO_H_ */
