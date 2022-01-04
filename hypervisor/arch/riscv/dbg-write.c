/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/sbi.h>
#include <jailhouse/types.h>
#include <jailhouse/console.h>
#include <jailhouse/control.h>
#include <jailhouse/printk.h>
#include <jailhouse/uart.h>

static void riscv_dbg_write_sbi(const char *msg)
{
	char ch;

	while ((ch = *msg++))
		sbi_console_putchar_legacy0_1(ch);
}

void arch_dbg_write_init(void)
{
	unsigned int con_type = system_config->debug_console.type;

	if (con_type == JAILHOUSE_CON_TYPE_RISCV_SBI)
		arch_dbg_write = riscv_dbg_write_sbi;
        else if (con_type == JAILHOUSE_CON_TYPE_8250)
                uart = &uart_8250_ops;

	if (uart) {
		uart->debug_console = &system_config->debug_console;
		uart->virt_base = hypervisor_header.debug_console_base;
		uart->init(uart);
		arch_dbg_write = uart_write;
	}
}
