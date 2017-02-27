/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2017
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <mach.h>
#include <inmate.h>
#include <gic.h>
#include <pmcnt.h>
#include <psci.h>

#define SECONDARY_CPU 3
#define SGI 5
#define SGIS_PER_SEC 10
#define PAGE_SIZE 0x1000

#define SGI_ISSUE_OTHER_CPUS() gic_issue_sgi(1, 0, SGI)

#if 0
#define SGI_ISSUE_A() SGI_ISSUE_OTHER_CPUS()
#define SGI_ISSUE_B() SGI_ISSUE_OTHER_CPUS()
#else
#define SGI_ISSUE_A() gic_issue_sgi(0, 0x4, SGI)
#define SGI_ISSUE_B() gic_issue_sgi(0, 0x8, SGI)
#endif

struct stack {
	unsigned char stack[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
	unsigned char irq[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
};

static struct stack cpu_b;

static u64 ticks_per_beat;

struct {
	unsigned int min;
	unsigned int avg;
	unsigned int max;
	unsigned long long sum;
	unsigned int n;
} rtt = {
	.min = -1,
};

static unsigned long long div_u64_u64(unsigned long long dividend,
				      unsigned long long divisor)
{
	unsigned long long result = 0;
	unsigned long long tmp_res, tmp_div;

	while (dividend >= divisor) {
		tmp_div = divisor << 1;
		tmp_res = 1;
		while (dividend >= tmp_div) {
			tmp_res <<= 1;
			if (tmp_div & (1ULL << 63))
				break;
			tmp_div <<= 1;
		}
		dividend -= divisor * tmp_res;
		result += tmp_res;
	}
	return result;
}

static void handle_IRQ(unsigned int irqn)
{
	unsigned int cnt = ccnt_read();
	unsigned int cpu = cpu_id();

	if (cpu == 0) {
		if (irqn == SGI) {
			/* print round trip time */
			rtt.n++;
			rtt.sum += cnt;
			rtt.avg = div_u64_u64(rtt.sum, rtt.n);
			if (cnt < rtt.min)
				rtt.min = cnt;
			if (cnt > rtt.max)
				rtt.max = cnt;
			printk("min: %lu avg: %lu max: %lu\n", rtt.min, rtt.avg, rtt.max);
		} else if (irqn == TIMER_IRQ) {
			timer_start(ticks_per_beat);
			ccnt_reset();
			SGI_ISSUE_B();
		}
	} else if (cpu == 1) {
		if (irqn == SGI)
			SGI_ISSUE_A();
	} else {
		printk("Interrupt on unknown CPU!\n");
	}
}

static void __attribute__((noreturn)) secondary_start(unsigned int processor_id)
{
	/* setup irq stack */
	asm volatile(".arch_extension virt\n\t"
		     "msr SP_irq, %0\n\t"
		     "cpsie i" : : "r"(cpu_b.irq+sizeof(cpu_b.irq)));
	gic_init();

	while(1)
		asm volatile("wfi");
}

void inmate_main(void)
{
	const unsigned int secondary_cpu = SECONDARY_CPU;
	unsigned long res;

	printk("Starting on processor %u...\n", cpu_id());

	printk("Initializing the GIC...\n");
	gic_setup(handle_IRQ);
	gic_enable_irq(TIMER_IRQ);

	printk("Starting secondary CPU...\n");
	res = psci_cpu_on(secondary_cpu, secondary_start, cpu_b.stack+sizeof(cpu_b.stack));
	if (res) {
		printk("Unable to start CPU %u\n", secondary_cpu);
		goto out;
	}

	ccnt_init();

	printk("Initializing SGI fire timer...\n");
	ticks_per_beat = timer_get_frequency() / SGIS_PER_SEC;
	timer_start(ticks_per_beat);

out:
	while (1)
		asm volatile("wfi" : : : "memory");
}
