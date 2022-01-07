/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/entry.h>
#include <asm/processor.h>
#include <asm/setup.h>

int arch_init_early(void)
{
	return -ENOSYS;
}

int arch_cpu_init(struct per_cpu *cpu_data)
{
	return -ENOSYS;
}

void __attribute__((noreturn))
riscv_deactivate_vmm(union registers *regs, int errcode)
{
	while (1);
}

void __attribute__((noreturn)) arch_cpu_activate_vmm(void)
{
	while (1);
}

void arch_cpu_restore(unsigned int cpu_id, int return_code)
{
}
