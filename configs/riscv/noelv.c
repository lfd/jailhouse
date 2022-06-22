/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for NOEL-V riscv64 target, 1G RAM, 6 cores
 *
 * Copyright (c) Siemens AG, 2020
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * NOTE: Add "mem=768M" to the kernel command line.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_system header;
	struct jailhouse_cpu cpus[6];
	struct jailhouse_memory mem_regions[7];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
	struct jailhouse_pci_capability pci_caps[0];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_RISCV64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x3fc00000,
			.size =       0x00400000,
		},

		.debug_console = {
			.type = JAILHOUSE_CON_TYPE_RISCV_SBI,
		},

		.platform_info = {
			.pci_mmconfig_base = 0x40000000,
			.pci_mmconfig_end_bus = 0,
			.pci_is_virtual = true,
			.pci_domain = 0,

			.riscv = {
				.irqchip = {
					.type = JAILHOUSE_RISCV_PLIC,
					.base_address = 0xf8000000,
					.size = 0x4000000,
					.max_irq = 31,
					.max_priority = 7,
					.hart_to_context = {
						[0] = 1, /* S-Mode of Hart 0 has PLIC context 1 */
						[1] = 5, /* S-Mode of Hart 1 has PLIC context 5 */
						[2] = 9, /* S-Mode of Hart 2 has PLIC context 9 */
						[3] = 13, /* S-Mode of Hart 3 has PLIC context 13 */
						[4] = 17, /* S-Mode of Hart 3 has PLIC context 17 */
						[5] = 21, /* S-Mode of Hart 3 has PLIC context 21 */
						[6 ... 31] = -1,
					},
				},
			},
		},

		.root_cell = {
			.name = "noelv-riscv64",
			.num_cpus = ARRAY_SIZE(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			.num_pci_caps = ARRAY_SIZE(config.pci_caps),

			/*
			 * This IRQ must be BELOW the .riscv.plic.max_irq, as
			 * Linux won't address any higher IRQ
			 */
			.vpci_irq_base = 28,
		},
	},

	.cpus = {
		{
			.phys_id = 1,
		},
		{
			.phys_id = 0,
		},
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
	},

	.mem_regions = {
		{ /* RAM */
			.phys_start = 0x0,
			.virt_start = 0x0,
			.size = 0x3fb00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		{ /* uart@fc001000 */
			.phys_start = 0xfc001000,
			.virt_start = 0xfc001000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		{ /* greth@fc084000 */
			.phys_start = 0xfc084000,
			.virt_start = 0xfc084000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* IVSHMEM shared memory regions (networking) */
		JAILHOUSE_SHMEM_NET_REGIONS(0x3fb00000, 0),
	},

	.irqchips = {
		/* plic@f8000000 */ {
			.address = 0xf8000000,
			.id = 0, /* PLIC */
			.pin_base = 0,
			.pin_bitmap = {
				(1 << 1) | /* UART */
				(1 << 5), /* greth */
				0,
				0,
				0,
			},
		},
	},

	.pci_devices = {
		{ /* IVSHMEM (networking) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0x0000,
			.bdf = 0x10 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 3,
			.shmem_dev_id = 0,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},
};
