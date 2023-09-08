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
 */

#define MIB			(1024UL * 1024UL)
#define GIB			(1024UL * MIB)

#define MEM_BASE		0x80000000UL
#define MEM_TOP			(MEM_BASE + 4 * GIB)

#define DTB_SIZE		(10 * MIB)
#define DTB_BASE		(MEM_TOP - DTB_SIZE)

#define HV_SIZE			(6 * MIB) /* 6MiB HV Size */
#define HV_PHYS			(DTB_BASE - HV_SIZE)

#define IVSHMEM_NET_PHYS	(HV_PHYS - MIB)

#define INMATE_TINY_SIZE	MIB
#define INMATE_TINY_PHYS	(IVSHMEM_NET_PHYS - INMATE_TINY_SIZE)

#define INMATE_LINUX_LOW_SIZE	(2 * MIB)
#define INMATE_LINUX_LOW_PHYS	(INMATE_TINY_PHYS - INMATE_LINUX_LOW_SIZE)

#define INMATE_LINUX_HIGH_SIZE	(128 * MIB)
#define INMATE_LINUX_HIGH_PHYS	(INMATE_LINUX_LOW_PHYS - INMATE_LINUX_HIGH_SIZE)

#define MEM_SIZE		(INMATE_LINUX_HIGH_PHYS - MEM_BASE)
