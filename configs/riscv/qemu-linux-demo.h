/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for RISC-V Linux inmate on QEMU
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
#include "qemu-layout.h"
#include "qemu-imsic.h"

#define MEM_REGIONS_BASE	4

#ifdef QEMU_IVSHMEM
#define IVSHMEM_REGIONS		4
#else
#define IVSHMEM_REGIONS		0
#endif /* !QEMU_IVSHMEM */

#ifdef QEMU_IMSIC
#define IMSIC_REGIONS		2
#else
#define IMSIC_REGIONS		0
#endif /* !QEMU_IMSIC */

#define MEM_REGIONS		(MEM_REGIONS_BASE + IVSHMEM_REGIONS + IMSIC_REGIONS)

struct {
	struct jailhouse_cell_desc cell;
	struct jailhouse_cpu cpus[2];
	struct jailhouse_memory mem_regions[MEM_REGIONS];
	struct jailhouse_irqchip irqchips[1];
#ifdef QEMU_IVSHMEM
	struct jailhouse_pci_device pci_devices[1];
#else
	struct jailhouse_pci_device pci_devices[0];
#endif
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_RISCV64,
		.name = "linux-demo",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG |
			JAILHOUSE_CELL_TEST_DEVICE |
			JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,

		.num_cpus = ARRAY_SIZE(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = ARRAY_SIZE(config.irqchips),
		.num_pci_devices = ARRAY_SIZE(config.pci_devices),

		.console = {
			.type = JAILHOUSE_CON_TYPE_8250,
			.size = 0x1000,
			.address = 0x10000000,
			.flags = JAILHOUSE_CON_ACCESS_MMIO | JAILHOUSE_CON_REGDIST_1,
		},

		.vpci_irq_base = 32,
	},

	.cpus = {
		{
			.phys_id = 0,
		},
		{
			.phys_id = 1,
		},
	},

	.mem_regions = {
#ifdef QEMU_IVSHMEM
		/* IVSHMEM shared memory regions (networking) */
		JAILHOUSE_SHMEM_NET_REGIONS(IVSHMEM_NET_PHYS, 1),
#endif
		/* RAM low */ {
			.phys_start = INMATE_LINUX_LOW_PHYS,
			.virt_start = 0x0,
			.size = INMATE_LINUX_LOW_SIZE,
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
			.phys_start = INMATE_LINUX_HIGH_PHYS,
			.virt_start = 0x80000000,
			.size = INMATE_LINUX_HIGH_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* uart@10000000 */ {
			.phys_start = 0x10000000,
			.virt_start = 0x10000000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
#ifdef QEMU_IMSIC
		IMSIC_NON_ROOT_REGION(0, VS_FILE),
		IMSIC_NON_ROOT_REGION(1, VS_FILE),
#endif
	},

	.pci_devices = {
#ifdef QEMU_IVSHMEM
		{ /* IVSHMEM (networking) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0x0000,
			.bdf = 0x10 << 3,
#ifdef QEMU_IMSIC
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_MSIX,
			.num_msix_vectors = 2,
#else
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
#endif
			.shmem_regions_start = 0,
			.shmem_dev_id = 1,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
#endif
	},

	.irqchips = {
		/* (A)PLIC */ {
#ifdef QEMU_PLIC
			.address = 0xc000000,
#elif defined(QEMU_APLIC)
			.address = 0xd000000,
#endif
			.pin_base = 0,
			.pin_bitmap = {
				(1 << 0xa), 0, 0, 0,
			},
		},
	},
};
