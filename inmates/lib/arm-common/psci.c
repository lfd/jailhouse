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
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <psci.h>
#include <asm/psci_call.h>
#include <asm/processor.h>

void (* volatile __c_entry)(void *);

extern void secondary_startup(void);

volatile unsigned int cpus_online;

unsigned int psci_version(void)
{
	return (unsigned int)psci_call(PSCI_VERSION, 0, 0, 0);
}

int psci_cpu_on(unsigned int cpu_id, void (*c_entry)(void *))
{
	int err;
	unsigned int cpus = cpus_online + 1;

	__c_entry = c_entry;
	memory_barrier();

	err = psci_call(PSCI_CPU_ON, cpu_id, (uintptr_t)secondary_startup, 0);
	if (err)
		return err;

	/* spin while CPU is not up */
	while (cpus != cpus_online)
		cpu_relax();

	return 0;
}

void __attribute__((noreturn)) psci_cpu_off(void)
{
	/* point of no return */
	psci_call(PSCI_CPU_OFF, 0, 0, 0);
	while (1)
		asm volatile("wfi");
}
