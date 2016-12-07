/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for linux-demo inmate on Jetson TK1:
 * 2 CPUs, 239M RAM, serial port D
 *
 * Copyright (c) Siemens AG, 2014-2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[11];
	struct jailhouse_irqchip irqchips[2];
	struct jailhouse_pci_device pci_devices[1];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.name = "jetson-tk1-linux-demo",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG|JAILHOUSE_CELL_DEBUG_CONSOLE,

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = ARRAY_SIZE(config.irqchips),
		.num_pci_devices = ARRAY_SIZE(config.pci_devices),

		.vpci_irq_base = 152,
	},

	.cpus = {
		0xc,
	},

	.mem_regions = {
		/* UART */ {
			.phys_start = 0x70006300,
			.virt_start = 0x70006300,
			.size = 0x40,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED |
				JAILHOUSE_MEM_IO_8,
		},
		/* RAM */ {
			.phys_start = 0xe0000000,
			.virt_start = 0,
			.size = 0x10000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA |
				JAILHOUSE_MEM_LOADABLE,
		},
		/* RAM */ {
			.phys_start = 0xe0010000,
			.virt_start = 0xe8000000,
			.size = 256 * 1024 * 1024,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA |
				JAILHOUSE_MEM_LOADABLE,
		},
		/* IVSHMEM shared memory region */ {
			.phys_start = 0xfbf00000,
			.virt_start = 0xfbf00000,
			.size = 0x100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* CAR */ {
			.phys_start = 0x60006000,
			.virt_start = 0x60006000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32 |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* I2C-0 */ {
			.phys_start = 0x7000c000,
			.virt_start = 0x7000c000,
			.size = 0x100,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* I2C-1 */ {
			.phys_start = 0x7000c400,
			.virt_start = 0x7000c400,
			.size = 0x100,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* APBDMA */ {
			.phys_start = 0x60020000,
			.virt_start = 0x60020000,
			.size = 0x2000, /* real size: 0x1400 */
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* SPI-0 */ {
			.phys_start = 0x7000d400,
			.virt_start = 0x7000d400,
			.size = 0x200,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* Pinmux */ {
			.phys_start = 0x70000000,
			.virt_start = 0x70000000,
			.size = 0x4000, /* pinmux has strange regions */
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* GPIO */ {
			.phys_start = 0x6000d000,
			.virt_start = 0x6000d000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
	},

	.irqchips = {
		/* GIC */ {
			.address = 0x50041000,
			.pin_base = 32,
			.pin_bitmap = {
				/* 38: I2C-0 Interrupt */
				/* 59: SPI@7000d400 */
				/* 84: I2C-1 Interrupt */
				/* 90: UART D Interrupt */
				/* 105: APBDMA 0.1 (for SPI) */
				0,
				(1 << (38 % 32)) | (1 << (59 % 32)),
				(1 << (84 % 32)) | (1 << (90 % 32)),
				(1 << (105 % 32)),
			},
		},
		/* GIC */ {
			.address = 0x50041000,
			.pin_base = 160,
			.pin_bitmap = {
				1 << (152+32 - 160),
			},
		},
	},

	.pci_devices = {
		/* 00:00.0 */ {
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.bdf = 0x00,
			.bar_mask = {
				0xffffff00, 0xffffffff, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.shmem_region = 3,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},
};
