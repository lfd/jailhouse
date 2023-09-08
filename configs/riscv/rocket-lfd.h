/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for LfD's RISC-V 64 Rocket chip (FPGA)
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>
#include "rocket-layout.h"

#define MEM_REGIONS_BASE	5

#ifdef ROCKET_IVSHMEM
#define IVSHMEM_REGIONS		4
#else
#define IVSHMEM_REGIONS		0
#endif /* ROCKET_IVSHMEM */

#define MEM_REGIONS		(MEM_REGIONS_BASE + IVSHMEM_REGIONS)

struct {
	struct jailhouse_system header;
#ifdef ROCKET_UC
	struct jailhouse_cpu cpus[1];
#elif defined(ROCKET_MC)
#if 0
	struct jailhouse_cpu cpus[8];
#else
	struct jailhouse_cpu cpus[2];
#endif
#endif
	struct jailhouse_memory mem_regions[MEM_REGIONS];
	struct jailhouse_irqchip irqchips[1];
#ifdef ROCKET_IVSHMEM
	struct jailhouse_pci_device pci_devices[1];
#else
	struct jailhouse_pci_device pci_devices[0];
#endif
	struct jailhouse_pci_capability pci_caps[0];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_RISCV64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			/* N.B.: 2 MiB megapage alignment requirement */
			.phys_start = HV_PHYS,
			.size =       HV_SIZE,
		},

		.debug_console = {
			/* Secondary UART, only used for Jailhouse */
			.address = 0x60020000,
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
			.divider = 0x51,
		},

		.platform_info = {
			.pci_mmconfig_base = 0x30000000,
			.pci_mmconfig_end_bus = 0,
			.pci_is_virtual = 1,
			.pci_domain = -1,

			.riscv = {
				.timebase_frequency = 1500000,
				.irqchip = {
					.type = JAILHOUSE_RISCV_PLIC,
					.base_address = 0xc000000,
					.size = 0x4000000,
					.max_irq = 96,
					.max_priority = 7,
					.hart_to_context = {
						[0] = 1,
						[1] = 3,
						[2] = 5,
						[3] = 7,
						[4] = 9,
						[5] = 11,
						[6] = 13,
						[7] = 15,
						[8 ... 31] = -1,
					},
				},
			},
		},
		.root_cell = {
			.name = "lfd-rocket",
			.num_cpus = ARRAY_SIZE(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			.num_pci_caps = ARRAY_SIZE(config.pci_caps),

			.vpci_irq_base = 32,
		},
	},

	.cpus = {
		{
			.phys_id = 0,
		},
#ifdef ROCKET_MC
		{
			.phys_id = 1,
		},
#if 0
		{
			.phys_id = 2,
		},
		{
			.phys_id = 3,
		},
		{
			.phys_id = 4,
		},
		{
			.phys_id = 5,
		},
		{
			.phys_id = 6,
		},
		{
			.phys_id = 7,
		},
#endif
#endif
	},

	.mem_regions = {
#ifdef ROCKET_IVSHMEM
		/* IVSHMEM shared memory regions (networking) */
		JAILHOUSE_SHMEM_NET_REGIONS(IVSHMEM_NET_PHYS, 0),
#endif
		/* RAM */ {
			.phys_start = MEM_BASE,
			.virt_start = MEM_BASE,
			.size = MEM_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* RAM - DTB */ {
			.phys_start = DTB_BASE,
			.virt_start = DTB_BASE,
			.size = DTB_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* uart0@60010000 */ {
			.phys_start = 0x60010000,
			.virt_start = 0x60010000,
			.size = 0x00010000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* uart1@60020000 */ {
			.phys_start = 0x60020000,
			.virt_start = 0x60020000,
			.size = 0x00010000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* ethernet@60030000 */ {
			.phys_start = 0x60030000,
			.virt_start = 0x60030000,
			.size = 0x00010000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
	},

	.irqchips = {
		/* plic@c000000 */ {
			.address = 0xc000000,
			.id = 0,
			.pin_base = 0,
			.pin_bitmap = {
				(1 << 1) | /* uart0@60010000 */
				(1 << 2) | /* uart1@60020000 */
				(1 << 3), /* ethernet@60030000 */
				0, 0, 0
			},
		},
	},

#ifdef ROCKET_IVSHMEM
	.pci_devices = {
		{ /* IVSHMEM (networking) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0,
			.bdf = 0x10 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 0,
			.shmem_dev_id = 0,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},
#endif
};
