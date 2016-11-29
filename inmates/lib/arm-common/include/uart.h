/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2016
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

struct uart_chip {
	void *base;

	void *clock_reg;
	int gate_nr;

	unsigned int divider;

	void (*init)(struct uart_chip*);
	void (*wait)(struct uart_chip*);
	void (*write)(struct uart_chip*, char c);
};
