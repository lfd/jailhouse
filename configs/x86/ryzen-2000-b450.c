/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2014-2017
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Configuration for Gigabyte Technology Co., Ltd. B450 AORUS M
 * created with './tools/jailhouse config create ./configs/x86/ryzen-2000-b450.c'
 *
 * NOTE: This config expects the following to be appended to your kernel cmdline
 *       "memmap=0x5200000$0x3a000000"
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[40];
	struct jailhouse_irqchip irqchips[2];
	__u8 pio_bitmap[0x2000];
	struct jailhouse_pci_device pci_devices[37];
	struct jailhouse_pci_capability pci_caps[95];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x3a000000,
			.size = 0x600000,
		},
		.debug_console = {
			.address = 0x3f8,
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_PIO |
				 JAILHOUSE_CON_REGDIST_1,
		},
		.platform_info = {
			.pci_mmconfig_base = 0xf8000000,
			.pci_mmconfig_end_bus = 0x3f,
			.x86 = {
				.pm_timer_address = 0x808,
				.vtd_interrupt_limit = 256,
				.iommu_units = {
					{
						.base = 0xfeb80000,
						.size = 0x80000,
						.amd_bdf = 0x2,
						.amd_base_cap = 0x40,
						.amd_msi_cap = 0x64,
						.amd_features = 0x80048f6f,
					},
				},
			},
		},
		.root_cell = {
			.name = "RootCell",
			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.pio_bitmap_size = ARRAY_SIZE(config.pio_bitmap),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			.num_pci_caps = ARRAY_SIZE(config.pci_caps),
		},
	},

	.cpus = {
		0x00000000000000ff,
	},

	.mem_regions = {
		/* MemRegion: 00000000-0009ffff : System RAM */
		{
			.phys_start = 0x0,
			.virt_start = 0x0,
			.size = 0xa0000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 00100000-011fffff : System RAM */
		{
			.phys_start = 0x100000,
			.virt_start = 0x100000,
			.size = 0x1100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 01200000-03ffffff : Kernel */
		{
			.phys_start = 0x1200000,
			.virt_start = 0x1200000,
			.size = 0x2e00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 04000000-09cfffff : System RAM */
		{
			.phys_start = 0x4000000,
			.virt_start = 0x4000000,
			.size = 0x5d00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 0a000000-0a1fffff : System RAM */
		{
			.phys_start = 0xa000000,
			.virt_start = 0xa000000,
			.size = 0x200000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 0a200000-0a209fff : ACPI Non-volatile Storage */
		{
			.phys_start = 0xa200000,
			.virt_start = 0xa200000,
			.size = 0xa000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: 0a20a000-0affffff : System RAM */
		{
			.phys_start = 0xa20a000,
			.virt_start = 0xa20a000,
			.size = 0xdf6000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 0b020000-39ffffff : System RAM */
		{
			.phys_start = 0xb020000,
			.virt_start = 0xb020000,
			.size = 0x2efe0000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 3f200000-d2bf0017 : System RAM */
		{
			.phys_start = 0x3f200000,
			.virt_start = 0x3f200000,
			.size = 0x939f1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: d2bf0018-d2c0e657 : System RAM */
		{
			.phys_start = 0xd2bf0018,
			.virt_start = 0xd2bf0018,
			.size = 0x1f000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: d2c0e658-d2c0f017 : System RAM */
		{
			.phys_start = 0xd2c0e658,
			.virt_start = 0xd2c0e658,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: d2c0f018-d2c1d057 : System RAM */
		{
			.phys_start = 0xd2c0f018,
			.virt_start = 0xd2c0f018,
			.size = 0xf000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: d2c1d058-dbf5efff : System RAM */
		{
			.phys_start = 0xd2c1d058,
			.virt_start = 0xd2c1d058,
			.size = 0x9342000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: dc087000-dc147fff : System RAM */
		{
			.phys_start = 0xdc087000,
			.virt_start = 0xdc087000,
			.size = 0xc1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: dc148000-dc259fff : ACPI Non-volatile Storage */
		{
			.phys_start = 0xdc148000,
			.virt_start = 0xdc148000,
			.size = 0x112000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: dcead000-deffffff : System RAM */
		{
			.phys_start = 0xdcead000,
			.virt_start = 0xdcead000,
			.size = 0x2153000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: e0000000-e02fffff : BOOTFB */
		{
			.phys_start = 0xe0000000,
			.virt_start = 0xe0000000,
			.size = 0x300000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: f0000000-f0007fff : 0000:04:00.0 */
		{
			.phys_start = 0xf0000000,
			.virt_start = 0xf0000000,
			.size = 0x8000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe200000-fe2fdfff : xhci-hcd */
		{
			.phys_start = 0xfe200000,
			.virt_start = 0xfe200000,
			.size = 0xfe000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe2ff000-fe2fffff : xhci-hcd */
		{
			.phys_start = 0xfe2ff000,
			.virt_start = 0xfe2ff000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe300000-fe3fffff : ccp */
		{
			.phys_start = 0xfe300000,
			.virt_start = 0xfe300000,
			.size = 0x100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe401000-fe401fff : ccp */
		{
			.phys_start = 0xfe401000,
			.virt_start = 0xfe401000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe500000-fe507fff : 0000:04:00.0 */
		{
			.phys_start = 0xfe500000,
			.virt_start = 0xfe500000,
			.size = 0x8000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe601000-fe603fff : 0000:03:00.0 */
		{
			.phys_start = 0xfe601000,
			.virt_start = 0xfe601000,
			.size = 0x3000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe604000-fe604fff : r8169 */
		{
			.phys_start = 0xfe604000,
			.virt_start = 0xfe604000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe700000-fe77ffff : 0000:01:00.1 */
		{
			.phys_start = 0xfe700000,
			.virt_start = 0xfe700000,
			.size = 0x80000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe780000-fe79ffff : ahci */
		{
			.phys_start = 0xfe780000,
			.virt_start = 0xfe780000,
			.size = 0x20000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe7a0000-fe7a1fff : xhci-hcd */
		{
			.phys_start = 0xfe7a0000,
			.virt_start = 0xfe7a0000,
			.size = 0x2000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe7a3000-fe7a7fff : xhci-hcd */
		{
			.phys_start = 0xfe7a3000,
			.virt_start = 0xfe7a3000,
			.size = 0x5000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe800000-fe807fff : ICH HD audio */
		{
			.phys_start = 0xfe800000,
			.virt_start = 0xfe800000,
			.size = 0x8000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe808000-fe808fff : ahci */
		{
			.phys_start = 0xfe808000,
			.virt_start = 0xfe808000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe900000-fe91ffff : 0000:06:00.0 */
		{
			.phys_start = 0xfe900000,
			.virt_start = 0xfe900000,
			.size = 0x20000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe920000-fe93ffff : 0000:06:00.0 */
		{
			.phys_start = 0xfe920000,
			.virt_start = 0xfe920000,
			.size = 0x20000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fe940000-fe943fff : ICH HD audio */
		{
			.phys_start = 0xfe940000,
			.virt_start = 0xfe940000,
			.size = 0x4000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: feb00000-feb00007 : SB800 TCO */
		{
			.phys_start = 0xfeb00000,
			.virt_start = 0xfeb00000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fed00000-fed003ff : HPET 0 */
		{
			.phys_start = 0xfed00000,
			.virt_start = 0xfed00000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: fedc0000-fedc0fff : pnp 00:06 */
		{
			.phys_start = 0xfedc0000,
			.virt_start = 0xfedc0000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* MemRegion: 100000000-41f37ffff : System RAM */
		{
			.phys_start = 0x100000000,
			.virt_start = 0x100000000,
			.size = 0x31f380000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 41f380000-41fffffff : RAM buffer */
		{
			.phys_start = 0x41f380000,
			.virt_start = 0x41f380000,
			.size = 0xc80000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA,
		},
		/* MemRegion: 3a600000-3f1fffff : JAILHOUSE Inmate Memory */
		{
			.phys_start = 0x3a600000,
			.virt_start = 0x3a600000,
			.size = 0x4c00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
	},

	.irqchips = {
		/* IOAPIC 9, GSI base 0 */
		{
			.address = 0xfec00000,
			.id = 0xa0,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
		/* IOAPIC 10, GSI base 24 */
		{
			.address = 0xfec01000,
			.id = 0x1,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
	},

	.pio_bitmap = {
		[   0x0/8 ...   0x3f/8] =   -1, /* 0000-003f */
		[  0x40/8 ...   0x47/8] = 0xf0, /* 0040-0043 : PIT */
		[  0x50/8 ...   0x57/8] =   -1, /* 0050-0053 : timer1 */

		/* 0060-0064 : NMI (HACK!) */
		/*      0064 : keyboard */
		[  0x60/8 ...   0x67/8] = 0xec, 

		[  0x70/8 ...   0x77/8] = 0xfc, /* 0070-0071 : RTC */
		[  0x80/8 ...   0x87/8] =   -1, /* 0080-0087 : dma page reg */
		[  0x88/8 ...   0x8f/8] =   -1, /* 0088-008f : dma page reg */
		[  0xa0/8 ...   0xa7/8] =   -1, /* 00a0-00a1 : pic2 */
		[  0xc0/8 ...   0xc7/8] =   -1, /* 00c0-00c7 : dma2 */
		[  0xc8/8 ...   0xcf/8] =   -1, /* 00c8-00cf : dma2 */
		[  0xd0/8 ...   0xd7/8] =   -1, /* 00d0-00d7 : dma2 */
		[  0xd8/8 ...   0xdf/8] =   -1, /* 00d8-00df : dma2 */
		[  0xf0/8 ...   0xf7/8] =   -1, /* 00f0-00f7 : fpu */
		[  0xf8/8 ...   0xff/8] =   -1, /* 00f8-00ff : fpu */
		[ 0x378/8 ...  0x37f/8] =   -1, /* 0378-037f : parport0 */
		[ 0x3b0/8 ...  0x3df/8] =    0, /* 03b0-03df : VGA */
		[ 0x3f8/8 ...  0x3ff/8] =   -1, /* 03f8-03ff : serial */
		[ 0x408/8 ...  0x40f/8] =   -1, /*      040b : pnp 00:06 */
		[ 0x4d0/8 ...  0x4d7/8] =   -1, /* 04d0-04d6 : pnp 00:06 */

		/* 0800-0803 : ACPI PM1a_EVT_BLK */
		/* 0804-0805 : ACPI PM1a_CNT_BLK */
		[ 0x800/8 ...  0x807/8] =   -1, 

		[ 0x808/8 ...  0x80f/8] =   -1, /* 0808-080b : ACPI PM_TMR */
		[ 0x810/8 ...  0x817/8] =   -1, /* 0810-0815 : ACPI CPU throttle */
		[ 0x820/8 ...  0x827/8] =   -1, /* 0820-0827 : ACPI GPE0_BLK */
		[ 0x900/8 ...  0x907/8] =   -1, /* 0900-0907 : pnp 00:06 */
		[ 0x908/8 ...  0x90f/8] =   -1, /* 0908-090f : pnp 00:06 */
		[ 0x910/8 ...  0x917/8] =   -1, /* 0910-0917 : pnp 00:06 */
		[ 0x918/8 ...  0x91f/8] =   -1, /* 0918-091f : pnp 00:06 */
		[ 0xa00/8 ...  0xa07/8] =   -1, /* 0a00-0a07 : pnp 00:02 */
		[ 0xa08/8 ...  0xa0f/8] =   -1, /* 0a08-0a0f : pnp 00:02 */
		[ 0xa10/8 ...  0xa17/8] =   -1, /* 0a10-0a17 : pnp 00:02 */
		[ 0xa18/8 ...  0xa1f/8] =   -1, /* 0a18-0a1f : pnp 00:02 */
		[ 0xa20/8 ...  0xa27/8] =   -1, /* 0a20-0a27 : pnp 00:02 */
		[ 0xa28/8 ...  0xa2f/8] =   -1, /* 0a28-0a2f : pnp 00:02 */
		[ 0xa30/8 ...  0xa37/8] =   -1, /* 0a30-0a37 : pnp 00:02 */
		[ 0xa38/8 ...  0xa3f/8] =   -1, /* 0a38-0a3f : pnp 00:02 */
		[ 0xa40/8 ...  0xa47/8] =   -1, /* 0a40-0a47 : pnp 00:02 */
		[ 0xa48/8 ...  0xa4f/8] =   -1, /* 0a48-0a4f : pnp 00:02 */
		[ 0xb00/8 ...  0xb0f/8] =   -1, /* 0b00-0b08 : piix4_smbus */
		[ 0xb20/8 ...  0xb27/8] =   -1, /* 0b20-0b27 : pnp 00:06 */
		[ 0xb28/8 ...  0xb2f/8] =   -1, /* 0b28-0b2f : pnp 00:06 */
		[ 0xb30/8 ...  0xb37/8] =   -1, /* 0b30-0b37 : pnp 00:06 */
		[ 0xb38/8 ...  0xb3f/8] =   -1, /* 0b38-0b3f : pnp 00:06 */
		[ 0xc00/8 ...  0xc07/8] =   -1, /* 0c00-0c01 : pnp 00:06 */
		[ 0xc10/8 ...  0xc17/8] =   -1, /*      0c14 : pnp 00:06 */
		[ 0xc50/8 ...  0xc57/8] =   -1, /* 0c50-0c52 : pnp 00:06 */
		[ 0xc68/8 ...  0xc6f/8] =   -1, /* 0c6c-0c6f : pnp 00:06 */
		[ 0xcd0/8 ...  0xcd7/8] =   -1, /* 0cd0-0cd7 : pnp 00:06 */
		[ 0xcd8/8 ...  0xcdf/8] =   -1, /* 0cd8-0cdf : pnp 00:06 */
		[ 0xcf8/8 ...  0xcff/8] =   -1, /* 0cf8-0cff : PCI conf1 */
		[ 0xd00/8 ... 0xffff/8] =    0, /* 0d00-ffff : PCI bus */
	},


	.pci_devices = {
		/* PCIDevice: 00:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.domain = 0x0,
			.bdf = 0x0,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:01.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x8,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:01.3 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xb,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 3,
			.num_caps = 11,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:02.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x10,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:03.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x18,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:03.1 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x19,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 3,
			.num_caps = 11,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:04.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x20,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:07.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x38,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:07.1 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x39,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 14,
			.num_caps = 9,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:08.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x40,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:08.1 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x41,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 14,
			.num_caps = 9,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:14.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xa0,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:14.3 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xa3,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc0,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.1 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc1,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.2 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc2,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.3 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc3,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.4 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc4,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.5 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc5,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.6 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc6,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 00:18.7 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0xc7,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 0,
			.num_caps = 0,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 01:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x100,
			.bar_mask = {
				0xffff8000, 0xffffffff, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 23,
			.num_caps = 8,
			.num_msi_vectors = 8,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 8,
			.msix_region_size = 0x1000,
			.msix_address = 0xfe7a2000,
		},
		/* PCIDevice: 01:00.1 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x101,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0xfffe0000,
			},
			.caps_start = 31,
			.num_caps = 4,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 01:00.2 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x102,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 35,
			.num_caps = 5,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 02:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x200,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 40,
			.num_caps = 7,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 02:01.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x208,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 40,
			.num_caps = 7,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 02:04.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_BRIDGE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x220,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 40,
			.num_caps = 7,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 03:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x300,
			.bar_mask = {
				0xffffff00, 0x00000000, 0xfffff000,
				0xffffffff, 0xffffc000, 0xffffffff,
			},
			.caps_start = 47,
			.num_caps = 9,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 4,
			.msix_region_size = 0x1000,
			.msix_address = 0xfe600000,
		},
		/* PCIDevice: 04:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x400,
			.bar_mask = {
				0xffffff00, 0xffff8000, 0xfffffffc,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 56,
			.num_caps = 4,
			.num_msi_vectors = 32,
			.msi_64bits = 1,
			.msi_maskable = 1,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 06:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x600,
			.bar_mask = {
				0xf0000000, 0xffffffff, 0xfffe0000,
				0xffffffff, 0xffffff00, 0x00000000,
			},
			.caps_start = 60,
			.num_caps = 5,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 06:00.1 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x601,
			.bar_mask = {
				0xffffc000, 0xffffffff, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 60,
			.num_caps = 5,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 07:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x700,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 65,
			.num_caps = 7,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 07:00.2 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x702,
			.bar_mask = {
				0x00000000, 0x00000000, 0xfff00000,
				0x00000000, 0x00000000, 0xffffe000,
			},
			.caps_start = 72,
			.num_caps = 8,
			.num_msi_vectors = 2,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 2,
			.msix_region_size = 0x1000,
			.msix_address = 0xfe400000,
		},
		/* PCIDevice: 07:00.3 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x703,
			.bar_mask = {
				0xfff00000, 0xffffffff, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 72,
			.num_caps = 8,
			.num_msi_vectors = 8,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 8,
			.msix_region_size = 0x1000,
			.msix_address = 0xfe2fe000,
		},
		/* PCIDevice: 08:00.0 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x800,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 65,
			.num_caps = 7,
			.num_msi_vectors = 0,
			.msi_64bits = 0,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 08:00.2 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x802,
			.bar_mask = {
				0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0xfffff000,
			},
			.caps_start = 80,
			.num_caps = 8,
			.num_msi_vectors = 16,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
		/* PCIDevice: 08:00.3 */
		{
			.type = JAILHOUSE_PCI_TYPE_DEVICE,
			.iommu = 0,
			.domain = 0x0,
			.bdf = 0x803,
			.bar_mask = {
				0xffff8000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.caps_start = 88,
			.num_caps = 7,
			.num_msi_vectors = 1,
			.msi_64bits = 1,
			.msi_maskable = 0,
			.num_msix_vectors = 0,
			.msix_region_size = 0x0,
			.msix_address = 0x0,
		},
	},

	.pci_caps = {
		/* PCIDevice: 00:00.2 */
		{
			.id = PCI_CAP_ID_SECDEV,
			.start = 0x40,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0x64,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_HT,
			.start = 0x74,
			.len = 2,
			.flags = 0,
		},
		/* PCIDevice: 00:01.3 */
		/* PCIDevice: 00:03.1 */
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x50,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x58,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0xa0,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_SSVID,
			.start = 0xc0,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_HT,
			.start = 0xc8,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_VNDR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x150,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_SECPCI | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x270,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ACS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x2a0,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_L1SS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x370,
			.len = 4,
			.flags = 0,
		},
		{
			.id = 0x23 | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x3c4,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 00:07.1 */
		/* PCIDevice: 00:08.1 */
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x50,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x58,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0xa0,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_SSVID,
			.start = 0xc0,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_HT,
			.start = 0xc8,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_VNDR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x150,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_SECPCI | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x270,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ACS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x2a0,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 01:00.0 */
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0x50,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_MSIX,
			.start = 0x68,
			.len = 12,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x78,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x80,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_SECPCI | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x200,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_LTR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x300,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_L1SS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x400,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 01:00.1 */
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0x50,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x78,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x80,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 01:00.2 */
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0x50,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x78,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x80,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_SSVID,
			.start = 0xc0,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 02:00.0 */
		/* PCIDevice: 02:01.0 */
		/* PCIDevice: 02:04.0 */
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0x50,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x78,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x80,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_SSVID,
			.start = 0xc0,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_SECPCI | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x200,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_L1SS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x400,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 03:00.0 */
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x40,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0x50,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x70,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSIX,
			.start = 0xb0,
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
			.id = PCI_EXT_CAP_ID_VC | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x140,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_DSN | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x160,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_LTR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x170,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_L1SS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x178,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 04:00.0 */
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x60,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0x68,
			.len = 24,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x80,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 06:00.0 */
		/* PCIDevice: 06:00.1 */
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x50,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x58,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0xa0,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_EXT_CAP_ID_VNDR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x150,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 07:00.0 */
		/* PCIDevice: 08:00.0 */
		{
			.id = PCI_CAP_ID_VNDR,
			.start = 0x48,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x50,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x64,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_VNDR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x150,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_SECPCI | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x270,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ACS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x2a0,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 07:00.2 */
		/* PCIDevice: 07:00.3 */
		{
			.id = PCI_CAP_ID_VNDR,
			.start = 0x48,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x50,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x64,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0xa0,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_MSIX,
			.start = 0xc0,
			.len = 12,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_EXT_CAP_ID_VNDR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x150,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ACS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x2a0,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 08:00.2 */
		{
			.id = PCI_CAP_ID_VNDR,
			.start = 0x48,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x50,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x64,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0xa0,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_SATA,
			.start = 0xd0,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_VNDR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x150,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ACS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x2a0,
			.len = 4,
			.flags = 0,
		},
		/* PCIDevice: 08:00.3 */
		{
			.id = PCI_CAP_ID_VNDR,
			.start = 0x48,
			.len = 2,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_PM,
			.start = 0x50,
			.len = 8,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_CAP_ID_EXP,
			.start = 0x64,
			.len = 60,
			.flags = 0,
		},
		{
			.id = PCI_CAP_ID_MSI,
			.start = 0xa0,
			.len = 14,
			.flags = JAILHOUSE_PCICAPS_WRITE,
		},
		{
			.id = PCI_EXT_CAP_ID_VNDR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x100,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ERR | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x150,
			.len = 4,
			.flags = 0,
		},
		{
			.id = PCI_EXT_CAP_ID_ACS | JAILHOUSE_PCI_EXT_CAP,
			.start = 0x2a0,
			.len = 4,
			.flags = 0,
		},
	},
};
