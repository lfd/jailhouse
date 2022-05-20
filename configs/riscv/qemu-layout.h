/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for QEMU riscv64 virtual target, 1G RAM, 6 cores
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define MIB1			(1024 * 1024)

#define MEM_TOP			0xC0000000

#define HV_SIZE			0x600000	/* 6MiB Size */
#define HV_PHYS			(MEM_TOP - HV_SIZE)

/* In preparation for IVSHMEM */
#define IVSHMEM_NET_PHYS	(HV_PHYS - MIB1)

#define INMATE_TINY_SIZE	MIB1
#define INMATE_TINY_PHYS	(IVSHMEM_NET_PHYS - INMATE_TINY_SIZE)

#define INMATE_LINUX_LOW_SIZE	(MIB1)
#define INMATE_LINUX_LOW_PHYS	(INMATE_TINY_PHYS - INMATE_LINUX_LOW_SIZE)

#define INMATE_LINUX_HIGH_SIZE	(128 * MIB1)
#define INMATE_LINUX_HIGH_PHYS	(INMATE_LINUX_LOW_PHYS - INMATE_LINUX_HIGH_SIZE)
