/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
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

#include <stdarg.h>
#include <inmate.h>

#define CON_TYPE		"PIO"
#define UART_BASE		0x3f8
#define UART_TX			0x0
#define UART_DLL		0x0
#define UART_DLM		0x1
#define UART_LCR		0x3
#define UART_LCR_8N1		0x03
#define UART_LCR_DLAB		0x80
#define UART_LSR		0x5
#define UART_LSR_THRE		0x20
#define UART_IDLE_LOOPS		100

static bool virtual_console;

static long unsigned int printk_uart_base;
static void (*uart_reg_out)(unsigned int, u8);
static u8 (*uart_reg_in)(unsigned int);
static void (*console_putc)(char c);

static void uart_pio_out(unsigned int reg, u8 value)
{
	outb(value, printk_uart_base + reg);
}

static u8 uart_pio_in(unsigned int reg)
{
	return inb(printk_uart_base + reg);
}

static void uart_mmio8_out(unsigned int reg, u8 value)
{
	mmio_write8((void *)printk_uart_base + reg, value);
}

static u8 uart_mmio8_in(unsigned int reg)
{
	return mmio_read8((void *)printk_uart_base + reg);
}

static void uart_mmio32_out(unsigned int reg, u8 value)
{
	mmio_write32((void *)printk_uart_base + reg * 4, value);
}

static u8 uart_mmio32_in(unsigned int reg)
{
	return mmio_read32((void *)printk_uart_base + reg * 4);
}

static void uart_putc(char c)
{
	while (!(uart_reg_in(UART_LSR) & UART_LSR_THRE))
		cpu_relax();
	uart_reg_out(UART_TX, c);
}

static void console_write(const char *msg)
{
	char c = 0;

	if (!console_putc && !virtual_console)
		return;

	while (1) {
		if (c == '\n')
			c = '\r';
		else
			c = *msg++;
		if (!c)
			break;

		if (console_putc)
			console_putc(c);

		if (virtual_console)
			jailhouse_call_arg1(JAILHOUSE_HC_DEBUG_CONSOLE_PUTC, c);
	}
}

#include "../../../hypervisor/printk-core.c"

static void console_init(void)
{
	const char *type;
	char buf[32];
	unsigned int divider, n;

	if (JAILHOUSE_CELL_HAS_DBG_PUTC(comm_region->flags)) {
		virtual_console =
			JAILHOUSE_CELL_HAS_DBG_ACTIVE(comm_region->flags);
		virtual_console =
			cmdline_parse_bool("con-virtual", virtual_console);
	}

	type = cmdline_parse_str("con-type", buf, sizeof(buf), CON_TYPE);
	printk_uart_base = cmdline_parse_int("con-base", UART_BASE);
	divider = cmdline_parse_int("con-divider", 0);

	if (strcmp(type, "PIO") == 0) {
		console_putc = uart_putc;
		uart_reg_out = uart_pio_out;
		uart_reg_in = uart_pio_in;
	} else if (strcmp(type, "MMIO8") == 0) {
		console_putc = uart_putc;
		uart_reg_out = uart_mmio8_out;
		uart_reg_in = uart_mmio8_in;
	} else if (strcmp(type, "MMIO32") == 0) {
		console_putc = uart_putc;
		uart_reg_out = uart_mmio32_out;
		uart_reg_in = uart_mmio32_in;
	} else {
		return;
	}

#ifdef __x86_64__
	if (strncmp(type, "MMIO", 4) == 0)
		map_range((void *)printk_uart_base, 0x1000, MAP_UNCACHED);
#endif

	if (divider > 0) {
		uart_reg_out(UART_LCR, UART_LCR_DLAB);
		uart_reg_out(UART_DLL, divider);
		uart_reg_out(UART_DLM, 0);
		uart_reg_out(UART_LCR, UART_LCR_8N1);
	} else {
		/*
		 * We share the UART with the hypervisor. Make sure all
		 * its outputs are done before starting.
		 */
		do {
			for (n = 0; n < UART_IDLE_LOOPS; n++)
				if (!(uart_reg_in(UART_LSR) & UART_LSR_THRE))
					break;
		} while (n < UART_IDLE_LOOPS);
	}
}

void printk(const char *fmt, ...)
{
	static bool inited;
	va_list ap;

	if (!inited) {
		console_init();
		inited = true;
	}

	va_start(ap, fmt);

	__vprintk(fmt, ap);

	va_end(ap);
}
