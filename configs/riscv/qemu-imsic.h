/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#define IMSIC_BASE		0x28000000
#define IMSIC_FILE_SIZE		0x1000

#define VS_FILE			1

/*
 * Qemu ist started with two guest files, the layout will be one page for:
 *   - S-File
 *   - VS-File 1
 *   - VS-File 2
 *   - Hole
 * This means, four pages in sum.
 */
#define IMSIC_HART_STRIDE	(4 * IMSIC_FILE_SIZE)

#define IMSIC_S_FILE(HART)	(IMSIC_BASE + (HART) * IMSIC_HART_STRIDE)
#define IMSIC_VS_FILE(HART, NO)	(IMSIC_S_FILE(HART) + (NO) * IMSIC_FILE_SIZE)

#define IMSIC_ROOT_REGION(HART, NO)					\
	{								\
		.phys_start = IMSIC_VS_FILE(HART, NO),			\
		.virt_start = IMSIC_S_FILE(HART),			\
		.size = 0x1000,						\
		.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,	\
	}

#define IMSIC_NON_ROOT_REGION(HART, NO)					\
	{								\
		.phys_start = IMSIC_VS_FILE(HART, NO),			\
		.virt_start = IMSIC_BASE + (HART) * IMSIC_FILE_SIZE,	\
		.size = IMSIC_FILE_SIZE,				\
		.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,	\
	}
