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

#define CAUSE_IRQ_FLAG	(1UL << (__riscv_xlen - 1))

void arch_handle_trap(void);

static inline bool is_irq(u64 cause)
{
	return !!(cause & CAUSE_IRQ_FLAG);
}

static inline unsigned long to_irq(unsigned long cause)
{
	return cause & ~CAUSE_IRQ_FLAG;
}

static int handle_irq(unsigned int irq)
{
	int err;
	struct sbiret ret;
	unsigned long tval;

	switch (irq) {
		case IRQ_S_TIMER:
			tval = timer_handler();
			ret = sbi_set_timer(tval);
			err = ret.error;
			break;

		default:
			err = -1;
			break;
	}
	return err;
}

/*
 * Any positive value will reprogramm the timer, -1 will halt the timer.
 */
unsigned long __attribute__((weak)) timer_handler(void)
{
	return -1;
}

void arch_handle_trap(void)
{
	unsigned long scause = csr_read(scause);
	int err;

	if (is_irq(scause)) {
		err = handle_irq(to_irq(scause));
		goto out;
	}

	switch (scause) {
	default:
		/* We don't have any exception handlers at the moment */
		printk("Unhandled exception %lu occured.\n", scause);
		err = -1;
		break;
	}

out:
	if (err) {
		printk("FATAL INMATE ERROR. HALTED.\n");
		stop();

	}
}
