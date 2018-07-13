/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 * Copyright (c) Siemens AG, 2014-2017
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <inmate.h>
#include <int.h>

#define TIMER_PERIOD	(100 * NS_PER_MSEC)

static volatile u64 expected;

/*
 * Enables blinking LED
 * Banana Pi:           register 0x1c2090c, pin 24
 * Orange Pi Zero:      register 0x1c20810, pin 17
 */
static void *led_reg;
static unsigned int led_pin;

static void handler_timer(void)
{
	static u64 min_delta = ~0ULL, max_delta = 0;
	u64 delta;

	delta = timer_get_ns() - expected;
	if (delta < min_delta)
		min_delta = delta;
	if (delta > max_delta)
		max_delta = delta;

	printk("Timer fired, jitter: %6ld ns, min: %6ld ns, max: %6ld ns\n",
	       (long)delta, (long)min_delta, (long)max_delta);

	if (led_reg)
		mmio_write32(led_reg, mmio_read32(led_reg) ^ (1 << led_pin));

	expected = timer_get_ns() + TIMER_PERIOD;
	timer_arm_expire(TIMER_PERIOD);
}

void inmate_main(void)
{
	int err;

	printk("Initializing the GIC...\n");
	timer_init();
	int_init();

	err = int_enable_irq(TIMER_IRQ, handler_timer);
	if (err) {
		printk("Unable to enable IRQ %u\n", TIMER_IRQ);
		goto out;
	}

	printk("Initializing the timer...\n");
	expected = timer_get_ns() + TIMER_PERIOD;
	timer_arm_expire(TIMER_PERIOD);

	led_reg = (void *)(unsigned long)cmdline_parse_int("led-reg", 0);
	led_pin = cmdline_parse_int("led-pin", 0);

out:
	halt();
}
