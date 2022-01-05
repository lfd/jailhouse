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

#define MIB			(1024UL * 1024UL)
#define GIB			(1024UL * MIB)

#define MEM_TOP			(0x80000000 + 1 * GIB)

#define HV_SIZE			0x600000 /* 6MiB HV Size */
#define HV_OFFSET		0x200000 /* Recent OpenSBI place DTB there */
#define HV_PHYS			(MEM_TOP - HV_OFFSET - HV_SIZE)
