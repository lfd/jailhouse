/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2017
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <psci.h>
#include <asm/processor.h>

#define PSCI_CPU_ON 0x84000003

volatile void *__cpu_stack_top;
volatile void *__c_entry;

void secondary_startup(void);

static inline long psci_call(unsigned long function_id, unsigned long arg0,
			     unsigned long arg1, unsigned long arg2)
{
	register __u32 __function_id asm("r0") = function_id;
	register __u32 __par1 asm("r1") = arg0;
	register __u32 __par2 asm("r2") = arg1;
	register __u32 __par3 asm("r3") = arg2;

	asm volatile(
		".arch_extension sec\n\t"
		"smc #0\n\t"
		: "=r" (__function_id)
		: "r"(__function_id), "r"(__par1), "r"(__par2), "r"(__par3)
		: "memory");

	return __function_id;
}

int psci_cpu_on(unsigned int cpu_id, void (*c_entry)(unsigned int), void *stack_top)
{
	int err;

	__cpu_stack_top = stack_top;
	__c_entry = c_entry;
	memory_barrier();

	err = psci_call(PSCI_CPU_ON, cpu_id, (unsigned long)secondary_startup, 0);
	if (err)
		return err;

	/* spin while CPU is not up */
	while (__cpu_stack_top)
		cpu_relax();

	return 0;
}
