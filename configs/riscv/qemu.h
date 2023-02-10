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
#include "qemu-layout.h"
#include "qemu-imsic.h"

#define MEM_REGIONS_BASE	9

#ifdef QEMU_IVSHMEM
#define IVSHMEM_REGIONS		4
#else
#define IVSHMEM_REGIONS		0
#endif /* QEMU_IVSHMEM */

#ifdef QEMU_IMSIC
#ifdef QEMU_UC
#define IMSIC_REGIONS		1
#else
#define IMSIC_REGIONS		6
#endif /* QEMU_UC */
#else
#define IMSIC_REGIONS		0
#endif /* QEMU_IMSIC */

#define MEM_REGIONS		(MEM_REGIONS_BASE + IVSHMEM_REGIONS + IMSIC_REGIONS)

struct {
	struct jailhouse_system header;
#ifdef QEMU_UC
	struct jailhouse_cpu cpus[1];
#elif defined(QEMU_MC)
	struct jailhouse_cpu cpus[6];
#endif
	struct jailhouse_memory mem_regions[MEM_REGIONS];
	struct jailhouse_irqchip irqchips[1];
#ifdef QEMU_IVSHMEM
	struct jailhouse_pci_device pci_devices[2];
#else
	struct jailhouse_pci_device pci_devices[1];
#endif
	struct jailhouse_pci_capability pci_caps[6];
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
			.type = JAILHOUSE_CON_TYPE_RISCV_SBI,
		},

		.platform_info = {
			.pci_mmconfig_base = 0x30000000,
			.pci_mmconfig_end_bus = 0xff, // ??
			.riscv = {
				.irqchip = {
#ifdef QEMU_PLIC
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
					},
#elif defined(QEMU_APLIC)
					.type = JAILHOUSE_RISCV_APLIC,
					.base_address = 0xd000000,
					.size = 0x8000,
					.max_irq = 96,
					.max_priority = 7,
#ifdef QEMU_IMSIC
					.imsic_base = IMSIC_BASE,
					.imsic_size = IMSIC_HART_STRIDE * ARRAY_SIZE(config.cpus),
					.imsic_stride = IMSIC_HART_STRIDE,
#endif
#endif
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
#ifdef QEMU_IVSHMEM
		/* IVSHMEM shared memory regions (networking) */
		JAILHOUSE_SHMEM_NET_REGIONS(IVSHMEM_NET_PHYS, 0),
#endif
		/* RAM */ {
			.phys_start = 0x80000000,
			.virt_start = 0x80000000,
			.size = 0x3f700000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* RAM - OpenSBI DTB */ {
			.phys_start = 0xbfe00000,
			.virt_start = 0xbfe00000,
			.size = 0x2000,
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

		/*
		 * When having an IMSIC, we hide the S-Mode file from the
		 * guest, and overlay it with a free VS-mode file.
		 *
		 * With IMSICs, every cell needs its own IMSIC VS-Mode file.
		 * Hence, the number of Jailhouse cells must be less or equal
		 * than the available VS-Mode files of the platform.
		 */
#ifdef QEMU_IMSIC
		IMSIC_ROOT_REGION(0, VS_FILE),
#ifdef QEMU_MC
		IMSIC_ROOT_REGION(1, VS_FILE),
		IMSIC_ROOT_REGION(2, VS_FILE),
		IMSIC_ROOT_REGION(3, VS_FILE),
		IMSIC_ROOT_REGION(4, VS_FILE),
		IMSIC_ROOT_REGION(5, VS_FILE),
#endif
#endif
	},
	.irqchips = {
#ifdef QEMU_PLIC
		/* plic@c000000 */ {
			.address = 0xc000000,
#elif defined(QEMU_APLIC)
		/* aplic_s@d000000 */ {
			.address = 0xd000000,
#endif
			.id = 0,
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
#ifdef QEMU_IVSHMEM
		{ /* IVSHMEM (networking) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0x0000,
			.bdf = 0x10 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 0,
			.shmem_dev_id = 0,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
#endif
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
