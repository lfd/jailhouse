/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for Linux inmate, 1 CPU, 74 MB RAM, ~1MB shmem, serial ports
 *
 * Copyright (c) Siemens AG, 2013-2015
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

#include "dell.h"

#define DACTALES_LOW_BASE	DACTALES_BASE
#define DACTALES_LOW_SIZE	0x100000 /* 1MiB */
#define DACTALES_HIGH_BASE	(DACTALES_LOW_BASE + DACTALES_LOW_SIZE)
#define DACTALES_HIGH_SIZE	(DACTALES_SIZE - DACTALES_LOW_SIZE)

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[4];
	struct jailhouse_cache cache_regions[0];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pio pio_regions[1];
	struct jailhouse_pci_device pci_devices[1];
	struct jailhouse_pci_capability pci_caps[0];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.name = "dactales-linux",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG |
			 JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_cache_regions = ARRAY_SIZE(config.cache_regions),
		.num_irqchips = ARRAY_SIZE(config.irqchips),
		.num_pio_regions = ARRAY_SIZE(config.pio_regions),
		.num_pci_devices = ARRAY_SIZE(config.pci_devices),
		.num_pci_caps = ARRAY_SIZE(config.pci_caps),
	},

	.cpus = {
		0x8,
	},

	.mem_regions = {
		/* low RAM */ {
			.phys_start = DACTALES_LOW_BASE,
			.virt_start = 0,
			.size = DACTALES_LOW_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA |
				JAILHOUSE_MEM_LOADABLE,
		},
		/* communication region */ {
			.virt_start = 0x00100000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
		/* high RAM */ {
			.phys_start = DACTALES_HIGH_BASE,
			.virt_start = 0x00200000,
			.size = DACTALES_HIGH_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA |
				JAILHOUSE_MEM_LOADABLE,
		},
		/* IVSHMEM shared memory region */
		{
			.phys_start = IVSHMEM_BASE,
			.virt_start = IVSHMEM_BASE,
			.size = IVSHMEM_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
	},

	.cache_regions = {
	},

	.irqchips = {
		/* IOAPIC */ {
			.address = 0xfec00000,
			.id = 0x3f0f8,
			.pin_bitmap = {
				(1 << 4),
			},
		},
	},

	.pio_regions = {
		PIO_RANGE(0x3f8, 8),
	},

	.pci_devices = {
		{
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0x0,
			.bdf = 0x08,
			.bar_mask = {
				0xffffff00, 0xffffffff, 0x00000000,
				0x00000000, 0xffffffe0, 0xffffffff,
			},
			.num_msix_vectors = 1,
			.shmem_region = 3,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},

	.pci_caps = {
	}
};
