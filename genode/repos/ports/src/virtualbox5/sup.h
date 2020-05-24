/*
 * \brief  Common VirtualBox SUPLib supplements
 * \author Norman Feske
 * \date   2013-08-20
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#ifndef _SUP_H_
#define _SUP_H_

/* Genode includes */
#include <base/component.h>
#include <cpu_session/connection.h>

/* VirtualBox includes */
#include <VBox/vmm/vm.h>
#include <VBox/vmm/gvmm.h>
#include <VBox/com/ptr.h>
#include <iprt/param.h>

#include "MachineImpl.h"
HRESULT genode_setup_machine(ComObjPtr<Machine> machine);

HRESULT genode_check_memory_config(ComObjPtr<Machine> machine, size_t);

/**
 * Returns true if a vCPU could be started. If false we run without
 * hardware acceleration support.
 */
bool create_emt_vcpu(pthread_t * pthread, size_t stack,
                     void *(*start_routine)(void *), void *arg,
                     Genode::Cpu_connection *,
                     Genode::Affinity::Location location,
                     unsigned int cpu_id,
                     const char * name, long prio);


uint64_t genode_cpu_hz();
void genode_update_tsc(void (*update_func)(void), Genode::uint64_t update_us);

Genode::Cpu_connection * get_vcpu_cpu_connection();

void genode_VMMR0_DO_GVMM_CREATE_VM(PSUPVMMR0REQHDR pReqHdr);
void genode_VMMR0_DO_GVMM_REGISTER_VMCPU(PVMR0 pVMR0, VMCPUID idCpu);

#endif /* _SUP_H_ */
