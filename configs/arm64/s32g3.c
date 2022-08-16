/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for S32G board
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Reservation is done via device tree entry:
 * reserved-memory {
 * 	jailhouse@0x880000000{
 * 		reg = <0x8 0x80000000 0x00 0x4000000>;
 *		no-map;
 *	};
 * };
 *
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>
#include "s32g3.h"

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[14];
	struct jailhouse_irqchip irqchips[2];
	struct jailhouse_pci_device pci_devices[1];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = JH_BASE,
			.size = JH_SIZE,
		},
		.debug_console = {
			.address = 0x401c8000,
			.size = 0x3000,
			.type = JAILHOUSE_CON_TYPE_LINFLEXUART,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			.pci_mmconfig_base = 0x30000000,
			.pci_mmconfig_end_bus = 0,
			.pci_is_virtual = 1,
			.pci_domain = -1,
			.arm = {
				.gic_version = 3,
				.gicd_base = 0x50800000,
				.gicr_base = 0x50900000,
				.maintenance_irq = 25,
			},
		},
		.root_cell = {
			.name = "S32G",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			
			.vpci_irq_base = 128 - 32,
		},
	},

	.cpus = {
		0xff,
	},

	.mem_regions = {
		JAILHOUSE_SHMEM_NET_REGIONS(SHMEM_NET_0_BASE, 0),

		/* I/O (permissive) */ {
			.phys_start = 0x40000000,
			.virt_start = 0x40000000,
			.size =	      0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},

		/* RAM */ {
			.phys_start = 0x80000000,
			.virt_start = 0x80000000,
			.size = 0x3200000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},

		/* pfebufs in between - access never seen */

		/* RAM */ {
			.phys_start = 0x83600000,
			.virt_start = 0x83600000,
			.size = 0xa00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},

		/* 0x84000000: s32cc-hse-rmem - access never seen */

		/* RAM */ {
			.phys_start = 0x84400000,
			.virt_start = 0x84400000,
			.size = 0xc0000000 - 0x84400000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},

		/* RAM */ {
			.phys_start = 0xc0800000,
			.virt_start = 0xc0800000,
			.size = 0xd0000000 - 0xc0800000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},

		/* SCMI SHMEM */ {
			.phys_start = 0xd0000000,
			.virt_start = 0xd0000000,
			.size =	      0x400000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},

		/* RAM */ {
			.phys_start = 0xd0400000,
			.virt_start = 0xd0400000,
			.size = 0x100000000 - 0xd0400000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},

		/* RAM (inmates) */ {
			.phys_start = INMATES_BASE,
			.virt_start = INMATES_BASE,
			.size = INMATES_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},

		/* RAM (upper RAM) */ {
			.phys_start = RESERVATION_END,
			.virt_start = RESERVATION_END,
			.size = RAM_UPPER_SIZE - RESERVATION_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},

		/* pcie */ {
			.phys_start = 0x5800000000,
			.virt_start = 0x5800000000,
			.size =	      0x800000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		}
	},

	.irqchips = {
		/* GIC */ {
			.address = 0x50800000,
			.pin_base = 32,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			},
		},
		/* GIC */ {
			.address = 0x50800000,
			.pin_base = 160,
			.pin_bitmap = {
				0xffffffff, 0xffffffff,
			},
		},
	},

	.pci_devices = {
		{ /* IVSHMEM 0000:00:01.0 (networking) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0,
			.bdf = 1 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 0,
			.shmem_dev_id = 0,
			.shmem_peers = 1,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},
};
