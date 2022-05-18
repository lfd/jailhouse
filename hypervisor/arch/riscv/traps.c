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

#include <asm/processor.h>

void arch_handle_trap(union registers *regs);
void arch_handle_fault(union registers *regs);

void arch_handle_trap(union registers *regs)
{
	for (;;)
		cpu_relax();
}

void arch_handle_fault(union registers *regs)
{
	for (;;)
		cpu_relax();
}
