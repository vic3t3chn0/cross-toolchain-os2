/*
 * \brief   Platform implementations specific for base-hw and i.MX8Q EVK
 * \author  Stefan Kalkowski
 * \date    2019-06-12
 */

/*
 * Copyright (C) 2019 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <platform.h>

/**
 * Leave out the first page (being 0x0) from bootstraps RAM allocator,
 * some code does not feel happy with addresses being zero
 */
Bootstrap::Platform::Board::Board()
: early_ram_regions(Memory_region { ::Board::RAM_BASE, ::Board::RAM_SIZE }),
  late_ram_regions(Memory_region { }),
  core_mmio(Memory_region { ::Board::UART_BASE, ::Board::UART_SIZE },
            Memory_region { ::Board::Cpu_mmio::IRQ_CONTROLLER_DISTR_BASE,
                            ::Board::Cpu_mmio::IRQ_CONTROLLER_DISTR_SIZE },
            Memory_region { ::Board::Cpu_mmio::IRQ_CONTROLLER_REDIST_BASE,
                            ::Board::Cpu_mmio::IRQ_CONTROLLER_REDIST_SIZE })
{
	::Board::Pic pic {};

	static volatile unsigned long initial_values[][2] {
		// GPC values
		{ 0x303A0004, 0x5050424  },
		{ 0x303A0030, 0xEB22DE22 },
		{ 0x303A0034, 0xFFFFF1C7 },
		{ 0x303A0038, 0x7BFFBC00 },
		{ 0x303A003C, 0xFA3BF12A },
		{ 0x303A004C, 0xFFFFDFFF },
		{ 0x303A01B4, 0x3980     },
		{ 0x303A01CC, 0xFFFFBFFF },
		{ 0x303A01D4, 0xFFFF7FFF },
		{ 0x303A01DC, 0xFFFF7FFF },
		{ 0x303A01FC, 0x107FF9F  },
		{ 0x303A080C, 0x1        },
		{ 0x303A0840, 0x1        },
		{ 0x303A084C, 0x1        },
		{ 0x303A0880, 0x1        },
		{ 0x303A088C, 0x1        },
		{ 0x303A08C0, 0x1        },
		{ 0x303A08CC, 0x1        },
		{ 0x303A0C8C, 0x1        },
		{ 0x303A0DCC, 0x1        },
		{ 0x303A0E0C, 0x1        },
		{ 0x303A0ECC, 0x1        },
		{ 0x303A0F00, 0x1        },
		{ 0x303A0F0C, 0x1        },

		// CCM values
		{ 0x303840B0, 0x2        },
		{ 0x303840B4, 0x2        },
		{ 0x303840B8, 0x2        },
		{ 0x303840BC, 0x2        },
		{ 0x303840C0, 0x2        },
		{ 0x303840C4, 0x2        },
		{ 0x303840C8, 0x2        },
		{ 0x303840CC, 0x2        },
		{ 0x303840D0, 0x2        },
		{ 0x303840D4, 0x2        },
		{ 0x303840D8, 0x2        },
		{ 0x303840DC, 0x2        },
		{ 0x303840E0, 0x2        },
		{ 0x303840E4, 0x2        },
		{ 0x303840E8, 0x2        },
		{ 0x303840EC, 0x2        },
		{ 0x303840F0, 0x2        },
		{ 0x303840F4, 0x2        },
		{ 0x303840F8, 0x2        },
		{ 0x303840FC, 0x2        },
		{ 0x30384250, 0x3        },
		{ 0x30384254, 0x3        },
		{ 0x30384258, 0x3        },
		{ 0x3038425C, 0x3        },
		{ 0x303843A0, 0x3        },
		{ 0x303843A4, 0x3        },
		{ 0x303843A8, 0x3        },
		{ 0x303843AC, 0x3        },
		{ 0x303844D0, 0x3        },
		{ 0x303844D4, 0x3        },
		{ 0x303844D8, 0x3        },
		{ 0x303844DC, 0x3        },
		{ 0x303844F0, 0x3        },
		{ 0x303844F4, 0x3        },
		{ 0x303844F8, 0x3        },
		{ 0x303844FC, 0x3        },
		{ 0x30384560, 0x0        },
		{ 0x30384564, 0x0        },
		{ 0x30384568, 0x0        },
		{ 0x3038456C, 0x0        },
		{ 0x303845D0, 0x3        },
		{ 0x303845D4, 0x3        },
		{ 0x303845D8, 0x3        },
		{ 0x303845DC, 0x3        },
		{ 0x30388010, 0x0        },
		{ 0x30388014, 0x0        },
		{ 0x30388018, 0x0        },
		{ 0x3038801C, 0x0        },
		{ 0x30388020, 0x0        },
		{ 0x30388024, 0x0        },
		{ 0x30388028, 0x0        },
		{ 0x3038802C, 0x0        },
		{ 0x30388030, 0x11000400 },
		{ 0x30388034, 0x11000400 },
		{ 0x30388038, 0x11000400 },
		{ 0x3038803C, 0x11000400 },
		{ 0x30388080, 0x11000000 },
		{ 0x30388084, 0x11000000 },
		{ 0x30388088, 0x11000000 },
		{ 0x3038808C, 0x11000000 },
		{ 0x30388090, 0x0        },
		{ 0x30388094, 0x0        },
		{ 0x30388098, 0x0        },
		{ 0x3038809C, 0x0        },
		{ 0x303880B0, 0x1100     },
		{ 0x303880B4, 0x1100     },
		{ 0x303880B8, 0x1100     },
		{ 0x303880BC, 0x1100     },
		{ 0x30388110, 0x0        },
		{ 0x30388114, 0x0        },
		{ 0x30388118, 0x0        },
		{ 0x3038811C, 0x0        },
		{ 0x30388180, 0x1000000  },
		{ 0x30388184, 0x1000000  },
		{ 0x30388188, 0x1000000  },
		{ 0x3038818C, 0x1000000  },
		{ 0x303881A0, 0x10000000 },
		{ 0x303881A4, 0x10000000 },
		{ 0x303881A8, 0x10000000 },
		{ 0x303881AC, 0x10000000 },
		{ 0x303881B0, 0x1000100  },
		{ 0x303881B4, 0x1000100  },
		{ 0x303881B8, 0x1000100  },
		{ 0x303881BC, 0x1000100  },
		{ 0x30388200, 0x1000000  },
		{ 0x30388204, 0x1000000  },
		{ 0x30388208, 0x1000000  },
		{ 0x3038820C, 0x1000000  },
		{ 0x30388220, 0x10000000 },
		{ 0x30388224, 0x10000000 },
		{ 0x30388228, 0x10000000 },
		{ 0x3038822C, 0x10000000 },
		{ 0x30388230, 0x1000100  },
		{ 0x30388234, 0x1000100  },
		{ 0x30388238, 0x1000100  },
		{ 0x3038823C, 0x1000100  },

		// CCMA values
		{ 0x30360000, 0x88080    },
		{ 0x30360004, 0x292A2FA6 },
		{ 0x30360004, 0x292A2FA6 },
		{ 0x30360008, 0x88080    },
		{ 0x30360008, 0x88080    },
		{ 0x3036000C, 0x10385BA3 },
		{ 0x3036000C, 0x10385BA3 },
		{ 0x30360010, 0x98080    },
		{ 0x30360010, 0x98080    },
		{ 0x30360014, 0x3FFFFF1A },
		{ 0x30360014, 0x3FFFFF1A },
		{ 0x30360018, 0x88081    },
		{ 0x30360054, 0x2B9      },

		// IOMUXC
		{ 0x30330064, 0x6        },
		{ 0x30330140, 0x0        },
		{ 0x30330144, 0x0        },
		{ 0x30330148, 0x0        },
		{ 0x3033014C, 0x0        },
		{ 0x30330150, 0x0        },
		{ 0x30330154, 0x0        },
		{ 0x30330158, 0x0        },
		{ 0x30330180, 0x2        },
		{ 0x30330184, 0x0        },
		{ 0x30330188, 0x0        },
		{ 0x3033018C, 0x0        },
		{ 0x30330190, 0x0        },
		{ 0x30330194, 0x0        },
		{ 0x30330198, 0x0        },
		{ 0x3033019C, 0x0        },
		{ 0x303301A0, 0x0        },
		{ 0x303301A4, 0x0        },
		{ 0x303301A8, 0x0        },
		{ 0x303301AC, 0x0        },
		{ 0x303301BC, 0x0        },
		{ 0x303301C0, 0x0        },
		{ 0x303301C4, 0x0        },
		{ 0x303301C8, 0x0        },
		{ 0x303301E8, 0x0        },
		{ 0x303301EC, 0x0        },
		{ 0x303301FC, 0x1        },
		{ 0x30330200, 0x1        },
		{ 0x3033021C, 0x10       },
		{ 0x30330220, 0x10       },
		{ 0x30330224, 0x10       },
		{ 0x30330228, 0x10       },
		{ 0x3033022C, 0x12       },
		{ 0x30330230, 0x12       },
		{ 0x30330244, 0x0        },
		{ 0x30330248, 0x0        },
		{ 0x3033029C, 0x19       },
		{ 0x303302A4, 0x19       },
		{ 0x303302A8, 0x19       },
		{ 0x303302B0, 0xD6       },
		{ 0x303302C0, 0x4F       },
		{ 0x303302C4, 0x16       },
		{ 0x303302CC, 0x59       },
		{ 0x3033033C, 0x9F       },
		{ 0x30330340, 0xDF       },
		{ 0x30330344, 0xDF       },
		{ 0x30330348, 0xDF       },
		{ 0x3033034C, 0xDF       },
		{ 0x30330350, 0xDF       },
		{ 0x30330368, 0x59       },
		{ 0x30330370, 0x19       },
		{ 0x3033039C, 0x19       },
		{ 0x303303A0, 0x19       },
		{ 0x303303A4, 0x19       },
		{ 0x303303A8, 0xD6       },
		{ 0x303303AC, 0xD6       },
		{ 0x303303B0, 0xD6       },
		{ 0x303303B4, 0xD6       },
		{ 0x303303B8, 0xD6       },
		{ 0x303303BC, 0xD6       },
		{ 0x303303C0, 0xD6       },
		{ 0x303303E8, 0xD6       },
		{ 0x303303EC, 0xD6       },
		{ 0x303303F0, 0xD6       },
		{ 0x303303F4, 0xD6       },
		{ 0x303303F8, 0xD6       },
		{ 0x303303FC, 0xD6       },
		{ 0x30330400, 0xD6       },
		{ 0x30330404, 0xD6       },
		{ 0x30330408, 0xD6       },
		{ 0x3033040C, 0xD6       },
		{ 0x30330410, 0xD6       },
		{ 0x30330414, 0xD6       },
		{ 0x30330424, 0xD6       },
		{ 0x30330428, 0xD6       },
		{ 0x3033042C, 0xD6       },
		{ 0x30330430, 0xD6       },
		{ 0x30330450, 0xD6       },
		{ 0x30330454, 0xD6       },
		{ 0x30330464, 0x49       },
		{ 0x30330468, 0x49       },
		{ 0x3033046C, 0x16       },
		{ 0x30330484, 0x67       },
		{ 0x30330488, 0x67       },
		{ 0x3033048C, 0x67       },
		{ 0x30330490, 0x67       },
		{ 0x30330494, 0x76       },
		{ 0x30330498, 0x76       },
		{ 0x303304AC, 0x49       },
		{ 0x303304B0, 0x49       },
		{ 0x303304C8, 0x1        },
		{ 0x303304CC, 0x4        },
		{ 0x30330500, 0x1        },
		{ 0x30330504, 0x2        },
		{ 0x30340038, 0x49409600 },
		{ 0x30340040, 0x49409200 }
	};

	unsigned num_values = sizeof(initial_values) / (2*sizeof(unsigned long));
	for (unsigned i = 0; i < num_values; i++)
		*((volatile Genode::uint32_t*)initial_values[i][0]) = (Genode::uint32_t)initial_values[i][1];
}


void Board::Cpu::wake_up_all_cpus(void * ip)
{
	enum Function_id { CPU_ON = 0xC4000003 };

	unsigned long result = 0;
	for (unsigned i = 1; i < NR_OF_CPUS; i++) {
		asm volatile("mov x0, %1  \n"
		             "mov x1, %2  \n"
		             "mov x2, %3  \n"
		             "mov x3, %2  \n"
		             "smc #0      \n"
		             "mov %0, x0  \n"
		             : "=r" (result) : "r" (CPU_ON), "r" (i), "r" (ip)
		                      : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
		                        "x8", "x9", "x10", "x11", "x12", "x13", "x14");
	}
}