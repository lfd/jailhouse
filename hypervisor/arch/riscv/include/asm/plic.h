/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _PLIC_H
#define _PLIC_H

#define PLIC_MAX_IRQS	1024

extern int plic_set_pending(void);
bool irqchip_irq_in_cell(struct cell *cell, unsigned int irq);

#endif /* _PLIC_H */
