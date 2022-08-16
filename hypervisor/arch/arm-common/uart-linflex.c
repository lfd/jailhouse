/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/mmio.h>
#include <jailhouse/uart.h>

#define UARTSR			0x0014
#define  LINFLEXD_UARTSR_DTFTFF	(1 << 1)
#define UART_BDRL		0x0038
#define  UART_STAT_TX_FULL	(1 << 11)

static void uart_init(struct uart_chip *chip)
{
}

static bool uart_is_busy(struct uart_chip *chip)
{
	return !!(mmio_read32(chip->virt_base + UARTSR) &
		LINFLEXD_UARTSR_DTFTFF);
}

static void uart_write_char(struct uart_chip *chip, char c)
{
	mmio_write8(chip->virt_base + UART_BDRL, c);
}

struct uart_chip uart_linflex_ops = {
	.init = uart_init,
	.is_busy = uart_is_busy,
	.write_char = uart_write_char,
};
