/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for QEMU riscv64 virtual target, 1G RAM, 6 cores
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define MIB			(1024UL * 1024UL)
#define GIB			(1024UL * MIB)

#define MEM_TOP			(0x80000000 + 1 * GIB)

/*
 * U-Boot puts the device-tree here, and this area must not be used. Jailhouse
 * needs to be located below that area.
 */
#define DTB_SIZE		(2 * MIB)
#define DTB_BASE		(MEM_TOP - DTB_SIZE)

#define HV_SIZE			(6 * MIB)
#define HV_PHYS			(DTB_BASE - HV_SIZE)

/* IVSHMEM only takes 1 MIB of space. Place it right below the hypervisor */
#define IVSHMEM_NET_PHYS	(HV_PHYS - 1 * MIB)

/*
 * Here we can place bootloader as well as tiny demos. Place it 1 MIB below
 * IVSHMEM. With this, we have a 2MIB alignment again, and can leverage huge
 * page alignment again
 */
#define INMATE_TINY_SIZE	MIB
#define INMATE_TINY_PHYS	(IVSHMEM_NET_PHYS - INMATE_TINY_SIZE)

#define INMATE_LINUX_LOW_SIZE	(2 * MIB)
#define INMATE_LINUX_LOW_PHYS	(INMATE_TINY_PHYS - INMATE_LINUX_LOW_SIZE)

#define INMATE_LINUX_HIGH_SIZE	(192 * MIB)
#define INMATE_LINUX_HIGH_PHYS	(INMATE_LINUX_LOW_PHYS - INMATE_LINUX_HIGH_SIZE)
