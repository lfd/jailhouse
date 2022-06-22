/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for RISC-V Linux inmate on NOEL-V
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
	struct jailhouse_cpu cpus[2];
	struct jailhouse_memory mem_regions[8];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.name = "linux-demo",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG |
			JAILHOUSE_CELL_TEST_DEVICE |
			JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,

		.num_cpus = ARRAY_SIZE(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_pci_devices = ARRAY_SIZE(config.pci_devices),
		.num_irqchips = ARRAY_SIZE(config.irqchips),

		/*
		 * This IRQ must be BELOW the .riscv.plic.max_irq, as
		 * Linux won't address any higher IRQ
		 */
		.vpci_irq_base = 28,
	},

	.cpus = {
		{
			.phys_id = 1,
		},
		{
			.phys_id = 2,
		},
	},

	.mem_regions = {
		/* RAM low */ {
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
		/* RAM high */ {
			.phys_start = 0x3ba00000,
			.virt_start = 0x80000000,
			.size = 0x4000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* uart@fc001000 */ {
			.phys_start = 0xfc001000,
			.virt_start = 0xfc001000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* IVSHMEM shared memory regions (networking) */
		JAILHOUSE_SHMEM_NET_REGIONS(0x3fb00000, 1),
	},

	.pci_devices = {
		{ /* IVSHMEM (networking) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0x0000,
			.bdf = 0x10 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 4,
			.shmem_dev_id = 1,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},

	.irqchips = {
		/* PLIC */ {
			.address = 0xf8000000,
			.pin_base = 0,
			.pin_bitmap = {
				(1 << 1), /* UART */
				0,
				0,
				0,
			},
		},
	},
};
