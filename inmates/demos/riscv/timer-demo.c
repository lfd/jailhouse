/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <inmate.h>

#include <jailhouse/hypercall.h>

static unsigned long timebase_frequency;
static unsigned long timer_next;

static inline u64 timer_ticks_to_ns(u64 ticks)
{
	return (1000UL * 1000UL * 1000UL * ticks) / timebase_frequency;
}

unsigned long timer_handler(void)
{
	static u64 min_delta = ~0ULL, max_delta = 0;
	unsigned long delta;

	delta = get_cycles() - timer_next;
	if (delta < min_delta)
		min_delta = delta;
	if (delta > max_delta)
		max_delta = delta;

	printk("Timer fired, jitter: %6ld ns, min: %6ld ns, max: %6ld ns\n",
		(long)timer_ticks_to_ns(delta),
		(long)timer_ticks_to_ns(min_delta),
		(long)timer_ticks_to_ns(max_delta));

	timer_next += timebase_frequency / 6;

	return timer_next;
}

void inmate_main(void)
{
	printk("Initializing the timer...\n");
	timebase_frequency = comm_region->timebase_frequency;
	timer_next = get_cycles() + 0x400000;
	sbi_set_timer(timer_next);
	timer_enable();
	enable_irqs();

	halt();
}
