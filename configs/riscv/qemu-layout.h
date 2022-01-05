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
