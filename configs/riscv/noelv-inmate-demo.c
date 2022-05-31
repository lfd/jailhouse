/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Minimal configuration for RISCV demo inmate, 1 CPU, 1MiB RAM
 *
 * Copyright (c) OTH Regensburg 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_cell_desc cell;
	struct jailhouse_cpu cpus[1];
	struct jailhouse_memory mem_regions[2];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_RISCV64,
		.name = "NOEL-V inmate demo",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG |
			JAILHOUSE_CELL_TEST_DEVICE |
			JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED |
			JAILHOUSE_CELL_VIRTUAL_CONSOLE_ACTIVE,

		.num_cpus = ARRAY_SIZE(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = 0,
		.num_pci_devices = 0,

		.console = {
			.type = JAILHOUSE_CON_TYPE_RISCV_SBI,
		},
	},

	.cpus = {
		{
			.phys_id = 1,
		},
	},

	.mem_regions = {
		/* RAM */ {
			.phys_start = 0x3fa00000,
			.virt_start = 0x0,
			.size = 0x100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* communication region */ {
			.virt_start = 0x00100000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
	},
};
