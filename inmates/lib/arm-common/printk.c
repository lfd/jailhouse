/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
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

#include <inmate.h>
#include <stdarg.h>
#include <uart.h>

#define UART_IDLE_LOOPS		100

static struct uart_chip *chip = NULL;
static bool virtual_console;

static void console_write(const char *msg)
{
	char c = 0;

	if (!chip && !virtual_console)
		return;

	while (1) {
		if (c == '\n')
			c = '\r';
		else
			c = *msg++;
		if (!c)
			break;

		if (chip) {
			while (chip->is_busy(chip))
				cpu_relax();
			chip->write(chip, c);
		}

		if (virtual_console)
			jailhouse_call_arg1(JAILHOUSE_HC_DEBUG_CONSOLE_PUTC, c);
	}
}

static void console_init(void)
{
	struct jailhouse_console *console = &comm_region->console;
	struct uart_chip **c;
	char buf[32];
	const char *type;
	unsigned int n;

	if (JAILHOUSE_COMM_HAS_DBG_PUTC_PERMITTED(comm_region->flags))
		virtual_console = cmdline_parse_bool("con-virtual",
			JAILHOUSE_COMM_HAS_DBG_PUTC_ACTIVE(comm_region->flags));

	type = cmdline_parse_str("con-type", buf, sizeof(buf), "");
	for (c = uart_array; *c; c++)
		if (!strcmp(type, (*c)->name) ||
		    (!*type && console->type == (*c)->type)) {
			chip = *c;
			break;
		}

	if (!chip)
		return;

	chip->base = (void *)(unsigned long)
		cmdline_parse_int("con-base", console->address);
	chip->divider = cmdline_parse_int("con-divider", console->divider);
	chip->gate_nr = cmdline_parse_int("con-gate-nr", console->gate_nr);
	chip->clock_reg = (void *)(unsigned long)
		cmdline_parse_int("con-clock-reg", console->clock_reg);

	if (chip->base)
		map_range(chip->base, PAGE_SIZE, MAP_UNCACHED);

	if (chip->clock_reg)
		map_range(chip->clock_reg, PAGE_SIZE, MAP_UNCACHED);

	chip->init(chip);

	if (chip->divider == 0) {
		/*
		 * We share the UART with the hypervisor. Make sure all
		 * its outputs are done before starting.
		 */
		do {
			for (n = 0; n < UART_IDLE_LOOPS; n++)
				if (chip->is_busy(chip))
					break;
		} while (n < UART_IDLE_LOOPS);
	}
}

#include "../../../hypervisor/printk-core.c"

void printk(const char *fmt, ...)
{
	static bool inited = false;
	va_list ap;

	if (!inited) {
		console_init();
		inited = true;
	}

	va_start(ap, fmt);

	__vprintk(fmt, ap);

	va_end(ap);
}
