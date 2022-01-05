/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for QEMU riscv64 virtual target, 1G RAM, 6 cores
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
#ifdef QEMU_UC
	struct jailhouse_cpu cpus[1];
#elif defined(QEMU_MC)
	struct jailhouse_cpu cpus[6];
#endif
	struct jailhouse_memory mem_regions[8];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
	struct jailhouse_pci_capability pci_caps[6];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_RISCV64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			/* N.B.: 2 MiB megapage alignment requirement */
			.phys_start = 0xbfa00000,
			.size =       0x00600000,
		},

		.debug_console = {
			.type = JAILHOUSE_CON_TYPE_RISCV_SBI,
		},

		.platform_info = {
			.pci_mmconfig_base = 0x30000000,
			.pci_mmconfig_end_bus = 0xff, // ??
			.riscv = {
				.irqchip = {
					.type = JAILHOUSE_RISCV_PLIC,
					.base_address = 0xc000000,
					.size = 0x600000,
					.max_irq = 96,
					.max_priority = 7,
					.hart_to_context = {
						[0] = 1,
						[1] = 3,
						[2] = 5,
						[3] = 7,
						[4] = 9,
						[5] = 11,
						[6 ... 31] = -1,
					}
				},
			},
		},
		.root_cell = {
			.name = "qemu-riscv64",
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
#ifdef QEMU_MC
		{
			.phys_id = 1,
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
#endif
	},

	.mem_regions = {
		/* RAM */ {
			.phys_start = 0x80000000,
			.virt_start = 0x80000000,
			.size = 0x3f900000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* rtc@101000 */ {
			.phys_start = 0x00101000,
			.virt_start = 0x00101000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* uart@10000000 */ {
			.phys_start = 0x10000000,
			.virt_start = 0x10000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* test@100000 */ {
			.phys_start = 0x00100000,
			.virt_start = 0x00100000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* flash@20000000 */ {
			.phys_start = 0x20000000,
			.virt_start = 0x20000000,
			.size = 0x02000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* flash@22000000, 2nd range */ {
			.phys_start = 0x22000000,
			.virt_start = 0x22000000,
			.size = 0x02000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* MemRegion: 40040000-4005ffff: e1000e */
		{
			.phys_start = 0x40040000,
			.virt_start = 0x40040000,
			.size = 0x20000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: 40060000-4007ffff: e1000e */
		{
			.phys_start = 0x40060000,
			.virt_start = 0x40060000,
			.size = 0x20000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
	},
	.irqchips = {
		/* plic@c000000 */ {
			.address = 0xc000000,
			.id = 0 /* PLIC */,
			.pin_base = 0,
			.pin_bitmap = {
				(1 << 0xa), /* uart@10000000 */
				(1 << (0x22 - 0x20)), /* PCI INT C / slot 0 */
				0,
				0,
			},
		},
	},

	.pci_devices = {
		{ /* e1000e */
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.domain = 0x0000,
			.bdf = 0x0010,
			.bar_mask = {
				0xfffe0000, 0xfffe0000, 0xffffffe0,
				0xffffc000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 6,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.num_msix_vectors = 5,
			.msix_region_size = 0x1000,
			.msix_address = 0x40080000,
		},
	},

	.pci_caps = {
		{ /* e1000e */
			.id = PCI_CAP_ID_PM,
			.start = 0xc8,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0xd0,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0xe0,
			.len = 20,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_MSIX,
			.start = 0xa0,
			.len = 12,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_DSN | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x140,
			.len = 4,
			.flags = 0,
		},
	},
};
