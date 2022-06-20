/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_ASM_CELL_H
#define _JAILHOUSE_ASM_CELL_H

#include <jailhouse/paging.h>
#include <jailhouse/types.h>

struct arch_cell {
	struct paging_structures mm;

	u32 irq_bitmap[MAX_IRQS / (sizeof(u32) * 8)];
	u32 virq_present_bitmap[MAX_IRQS / (sizeof(u32) * 8)];
};

#endif /* !_JAILHOUSE_ASM_CELL_H */
