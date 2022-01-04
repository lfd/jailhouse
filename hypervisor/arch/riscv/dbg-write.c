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

#include <jailhouse/types.h>
#include <jailhouse/console.h>
#include <jailhouse/control.h>
#include <jailhouse/printk.h>
#include <asm/sbi.h>

static void riscv_dbg_write_sbi(const char *msg)
{
	char ch;

	while ((ch = *msg++))
		sbi_console_putchar_legacy0_1(ch);
}

void arch_dbg_write_init(void)
{
	u32 dbg_type = system_config->debug_console.type;

	if (dbg_type == JAILHOUSE_CON_TYPE_RISCV_SBI)
		arch_dbg_write = riscv_dbg_write_sbi;
}
