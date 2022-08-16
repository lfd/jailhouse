/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * Configuration for S32G board
 *
 * Configuration for demo inmate on S32G3:
 * 1 CPU, 1 MiB RAM, 1 serial port
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>
#include "s32g3.h"

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[3];
} __attribute__((packed)) config = {
	.cell = {
		.architecture = JAILHOUSE_ARM64,
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.name = "s32g-inmate-demo",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG | JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),

		.console = {
			.address = 0x401c8000,
			.type = JAILHOUSE_CON_TYPE_LINFLEXUART,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0x80,
	},

	.mem_regions = {
		/* UART */ {
			.phys_start = 0x401c8000,
			.virt_start = 0x401c8000,
			.size = 0x3000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* RAM */ {
			.phys_start = INMATES_BASE,
			.virt_start = 0,
			.size = INMATES_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* communication region */ {
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
	},
};
