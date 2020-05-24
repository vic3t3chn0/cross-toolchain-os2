/*
 * \brief  VirtualBox hardware-acceleration manager
 * \author Norman Feske
 * \date   2013-08-20
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/log.h>

/* VirtualBox includes */
#include "HMInternal.h" /* enable access to hm.s.* */
#include <VBox/vmm/hm.h>
#include <VBox/vmm/vm.h>

/* Genode's VirtualBox includes */
#include "sup.h"

enum { VERBOSE_HM = false };

static bool enabled_hm    = true;
static bool enable_pae_nx = false;
static bool enable_64bit  = false;

VMMR3DECL(int) HMR3Init(PVM pVM)
{
	PCFGMNODE pCfgHM = CFGMR3GetChild(CFGMR3GetRoot(pVM), "HM/");

	/* check whether to stay for non-paged modi in recompiler */
	int rc = CFGMR3QueryBoolDef(pCfgHM, "EnableUX",
	                            &pVM->hm.s.vmx.fAllowUnrestricted, true);
	AssertRCReturn(rc, rc);

	/* check whether to enable pae and nx bit - in 64bit host mode */
	rc = CFGMR3QueryBoolDef(CFGMR3GetRoot(pVM), "EnablePAE", &enable_pae_nx,
	                        false);
	AssertRCReturn(rc, rc);

	/* check whether to enable long-mode bit - in 64bit host mode */
	rc = CFGMR3QueryBoolDef(pCfgHM, "64bitEnabled", &enable_64bit, false);
	AssertRCReturn(rc, rc);

	/*
	 * We always set the fHMEnabled flag. Otherwise, the EM won't
	 * consult us for taking scheduling decisions. The actual switch to
	 * HW accelerated mode is still dependent on the result of the
	 * HMR3CanExecuteGuest function.
	 */
	pVM->fHMEnabled = true;

	for (VMCPUID i = 0; i < pVM->cCpus; i++)
		pVM->aCpus[i].hm.s.fActive = false;

#if HC_ARCH_BITS == 64
	PGMSetLargePageUsage(pVM, true);
#endif

	/* XXX check for intel and feature bits XXX */
	if (pVM->hm.s.vmx.fAllowUnrestricted)
		pVM->hm.s.vmx.fUnrestrictedGuest = true;

	pVM->fHMEnabledFixed = true;
	pVM->hm.s.fNestedPaging = true;

	return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) HMR3Term(PVM pVM)
{
	return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) HMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat)
{
	enabled_hm = pVM->hm.s.svm.fSupported || pVM->hm.s.vmx.fSupported;

	if (!enabled_hm || enmWhat != VMINITCOMPLETED_RING0)
		return VINF_SUCCESS;

	int rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0 /*idCpu*/, VMMR0_DO_HM_SETUP_VM, 0, NULL);

	if (rc == VINF_SUCCESS) {
		CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_SEP);

		/* nova kernel supports solely on 64bit the following features */
		if (sizeof(void *) > 4 && enable_pae_nx) {
			CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_PAE);
			CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_NX);
		}
		if (sizeof(void *) > 4 && enable_64bit) {
			CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_LONG_MODE);
			CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_SYSCALL);
	        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_LAHF);
		}
	}

	return rc;
}


VMMR3DECL(bool) HMR3IsVmxPreemptionTimerUsed(PVM pVM)
{
	if (VERBOSE_HM)
		Genode::log(__func__, " called");

	return false;
}


VMMR3DECL(bool) HMR3IsActive(PVMCPU pVCpu)
{
	return pVCpu->hm.s.fActive;
}


VMMR3DECL(bool) HMR3IsRescheduleRequired(PVM pVM, PCPUMCTX pCtx)
{
	if (pVM->hm.s.vmx.fAllowUnrestricted)
		return false;

	return !CPUMIsGuestInPagedProtectedModeEx(pCtx);
}


VMMR3DECL(bool) HMR3IsEventPending(PVMCPU pVCpu)
{
	return  HMIsEnabled(pVCpu->pVMR3) && pVCpu->hm.s.Event.fPending;
}


VMMR3DECL(bool) HMR3CanExecuteGuest(PVM pVM, PCPUMCTX pCtx)
{
	if (!enabled_hm)
		return false;

	PVMCPU pVCpu = VMMGetCpu(pVM);

	if (pVM->hm.s.vmx.fAllowUnrestricted) {
		pVCpu->hm.s.fActive = true;
	} else
		/* enable H/W acceleration in protected and paged mode only */
		pVCpu->hm.s.fActive = CPUMIsGuestInPagedProtectedModeEx(pCtx);

	return pVCpu->hm.s.fActive;
}

VMMR3_INT_DECL(void) HMR3ResetCpu(PVMCPU pVCpu)
{
	pVCpu->hm.s.fActive = false;
}

extern "C" void PGMFlushVMMemory();
VMMR3_INT_DECL(void) HMR3Reset(PVM pVM)
{
	PGMFlushVMMemory();
}

VMMR3_INT_DECL(void) HMR3PagingModeChanged(PVM pVM, PVMCPU pVCpu, PGMMODE enmShadowMode, PGMMODE enmGuestMode)
{
}

/**
 * Restarting I/O instruction refused in ring-0 - no mean for us
 */
VBOXSTRICTRC HMR3RestartPendingIOInstr(PVM, PVMCPU pVCpu, PCPUMCTX)
{
	HMPENDINGIO enmType = pVCpu->hm.s.PendingIO.enmType;
	if (enmType != HMPENDINGIO_INVALID)
		Genode::error("unimplemented: restart pending I/O");
	return VERR_NOT_FOUND;
}


VMMR3DECL(bool) HMR3IsPostedIntrsEnabled(PUVM pUVM)
{
	Genode::log(__func__, " called");
	return false;
}


VMMR3DECL(bool) HMR3IsVirtApicRegsEnabled(PUVM pUVM)
{
	Genode::log(__func__, " called");
	return false;
}
