/*
 * \brief   Initial pagetables for x86_64
 * \author  Adrian-Ken Rueegsegger
 * \author  Reto Buerki
 * \author  Stefan Kalkowski
 * \date    2015-04-22
 */

/*
 * Copyright (C) 2015-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

.set MIN_PAGE_SIZE_LOG2, 12

.data

	/********************************************
	 ** Identity mapping from 4KiB to 1GiB     **
	 ** plus mappings for LAPIC, I/O APIC MMIO **
	 ** Page 0 containing the Bios Data Area   **
	 ** gets mapped to 2MiB - 4KiB readonly.   **
	 ********************************************/

	/* PML4 */
	.p2align MIN_PAGE_SIZE_LOG2
	.global _kernel_pml4
	_kernel_pml4:
	.quad _kernel_pdp + 0xf
	.fill 511, 8, 0x0

	/* PDP */
	.p2align MIN_PAGE_SIZE_LOG2
	_kernel_pdp:
	.quad _kernel_pd_0 + 0xf
	.fill 1, 8, 0x0
	.quad _kernel_pd_2 + 0xf
	.quad _kernel_pd_3 + 0xf
	.fill 508, 8, 0x0

	/* PD [0G-1G) */
	.p2align MIN_PAGE_SIZE_LOG2
	_kernel_pd_0:
	.quad _kernel_pt_bda + 0xf
	.set entry, 0x20018f
	.rept 511
	.quad entry
	.set entry, entry + 0x200000
	.endr

	/* PD [2G-3G) */
	.p2align MIN_PAGE_SIZE_LOG2
	_kernel_pd_2:
	.set entry, 0x8000018f
	.rept 512
	.quad entry
	.set entry, entry + 0x200000
	.endr

	/* PD [3G-4G) */
	.p2align MIN_PAGE_SIZE_LOG2
	_kernel_pd_3:
	.set entry, 0xc000018f
	.rept 502
	.quad entry
	.set entry, entry + 0x200000
	.endr
	.quad 0xfec0019f
	.quad 0xfee0019f
	.fill 8, 8, 0x0

	.p2align MIN_PAGE_SIZE_LOG2
	_kernel_pt_bda:
	.fill 1, 8, 0x0
	.set entry, 0x118f
	.rept 510
	.quad entry
	.set entry, entry + 0x1000
	.endr
	.quad 0x000001
