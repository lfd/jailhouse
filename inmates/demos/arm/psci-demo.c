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

#include <mach.h>
#include <inmate.h>
#include <gic.h>
#include <psci.h>

#ifdef CONFIG_BARE_METAL
#define PRIMARY_CPU   0
#define SECONDARY_CPU 1
#else
#define PRIMARY_CPU   2
#define SECONDARY_CPU 3
#endif

#define SGI 5

#define SGI_ISSUE_A() gic_issue_sgi(0, 1 << PRIMARY_CPU, SGI)
#define SGI_ISSUE_B() gic_issue_sgi(0, 1 << SECONDARY_CPU, SGI)

static void handle_IRQ(unsigned int irqn)
{
	static unsigned int ctr = 0;
	unsigned int my_cpu;

	if (!is_sgi(irqn))
		return;

	if (irqn != SGI)
		return;

	my_cpu = cpu_id();

	printk("SGI %u on CPU %u\n", irqn, my_cpu);

	if (ctr == 100)
		return;

	if (++ctr == 100) {
		printk("Offlining CPU %d\n", my_cpu);
		psci_cpu_off();
	}

	if (my_cpu == PRIMARY_CPU)
		SGI_ISSUE_B();
	else if (my_cpu == SECONDARY_CPU)
		SGI_ISSUE_A();
	else
		printk("Interrupt on unknown CPU!\n");
}

static void __attribute__((noreturn)) secondary_start(void *irq_stack)
{
	const unsigned int id = cpu_id();

	printk("Hello world from CPU %d\n", id);
	gic_setup(handle_IRQ, irq_stack);

	printk("Sending first SGI from CPU %d to CPU %d...\n", id, PRIMARY_CPU);
	SGI_ISSUE_A();

	while(1)
		asm volatile("wfi");
}

void inmate_main(void *irq_stack)
{
	unsigned long res;

	printk("Starting on processor %u...\n", cpu_id());
	printk("PSCI version: %x\n", psci_version());
	printk("Initializing the GIC...\n");
	gic_setup(handle_IRQ, irq_stack);
	printk("Starting secondary CPU %d via PSCI...\n", SECONDARY_CPU);
	res = psci_cpu_on(SECONDARY_CPU, secondary_start);
	if (res)
		printk("Unable to start CPU %u: %d\n", SECONDARY_CPU, res);

	while (1)
		asm volatile("wfi");
}
