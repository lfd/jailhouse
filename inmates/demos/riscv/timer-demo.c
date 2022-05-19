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

#include <inmate.h>

#include <jailhouse/hypercall.h>

const unsigned long timer_delta = 0x400000;

unsigned long timer_next;

static inline u64 timer_ticks_to_ns(u64 ticks)
{
	/* FIXME: implement me */
	return ticks;
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

	timer_next += timer_delta;

	return timer_next;
}

void inmate_main(void)
{
	printk("Initializing the timer...\n");
	timer_next = get_cycles() + 0x400000;
	sbi_set_timer(timer_next);
	timer_enable();
	enable_irqs();

	halt();
}
