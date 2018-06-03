/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013-2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <inmate.h>
#include <alloc.h>
#include <cmdline.h>
#include <int.h>
#include <timer.h>
#include <layout.h>
#include <mem.h>
#include <printk.h>

#define POLLUTE_CACHE_SIZE	(512 * 1024)

static unsigned long expected_time;
static unsigned long min = -1, max;

static void irq_handler(void)
{
	unsigned long delta;

	delta = timer_get_ns() - expected_time;
	if (delta < min)
		min = delta;
	if (delta > max)
		max = delta;
	printk("Timer fired, jitter: %6ld ns, min: %6ld ns, max: %6ld ns\n",
	       delta, min, max);

	expected_time += 100 * NS_PER_MSEC;
	timer_arm_expire(expected_time - timer_get_ns());
}

static void pollute_cache(char *mem)
{
	unsigned long cpu_cache_line_size, ebx;
	unsigned long n;

	asm volatile("cpuid" : "=b" (ebx) : "a" (1)
		: "rcx", "rdx", "memory");
	cpu_cache_line_size = (ebx & 0xff00) >> 5;

	for (n = 0; n < POLLUTE_CACHE_SIZE; n += cpu_cache_line_size)
		mem[n] ^= 0xAA;
}

void inmate_main(void)
{
	bool allow_terminate = false;
	bool terminate = false;
	bool cache_pollution;
	char *mem;

	comm_region->cell_state = JAILHOUSE_CELL_RUNNING_LOCKED;

	cache_pollution = cmdline_parse_bool("pollute-cache", false);
	if (cache_pollution) {
		mem = alloc(PAGE_SIZE, PAGE_SIZE);
		printk("Cache pollution enabled\n");
	}

	int_init();
	int_enable_irq(TIMER_IRQ, irq_handler);

	timer_init();

	expected_time = timer_get_ns() + NS_PER_MSEC;
	timer_arm_expire(NS_PER_MSEC);

	asm volatile("sti");

	while (!terminate) {
		asm volatile("hlt");

		if (cache_pollution)
			pollute_cache(mem);

		switch (comm_region->msg_to_cell) {
		case JAILHOUSE_MSG_SHUTDOWN_REQUEST:
			if (!allow_terminate) {
				printk("Rejecting first shutdown request - "
				       "try again!\n");
				jailhouse_send_reply_from_cell(comm_region,
						JAILHOUSE_MSG_REQUEST_DENIED);
				allow_terminate = true;
			} else
				terminate = true;
			break;
		default:
			jailhouse_send_reply_from_cell(comm_region,
					JAILHOUSE_MSG_UNKNOWN);
			break;
		}
	}

	printk("Stopped APIC demo\n");
	comm_region->cell_state = JAILHOUSE_CELL_SHUT_DOWN;
}
