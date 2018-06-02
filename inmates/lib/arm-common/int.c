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

#include <alloc.h>
#include <gic.h>
#include <int.h>
#include <layout.h>
#include <printk.h>
#include <string.h>

#define NUM_IRQS	128

extern const struct gic gic_v2;
extern const struct gic gic_v3;

static int_handler_t *int_handlers;
static const struct gic *gic = &gic_v2;

/* Replaces the weak reference in header.S */
void vector_irq(void)
{
	u32 irqn;

	while (1) {
		irqn = gic->read_ack();
		if (irqn == 0x3ff)
			break;

		if (irqn < NUM_IRQS && int_handlers[irqn])
			int_handlers[irqn]();
		else
			printk("WARNING: unable to handle IRQ %u\n", irqn);

		gic->write_eoi(irqn);
	}
}

void int_init(void)
{
	if (comm_region->gic_version == 3)
		gic = &gic_v3;

	int_handlers = alloc(NUM_IRQS * sizeof(int_handler_t),
			     sizeof(int_handler_t));
	memset(int_handlers, 0, NUM_IRQS * sizeof(int_handler_t));

	gic->init();
	gic_setup_irq_stack();
}

int int_enable_irq(unsigned int irq, int_handler_t handler)
{
	if (irq >= NUM_IRQS)
		return -1;

	int_handlers[irq] = handler;

	if (!is_sgi(irq))
		gic->enable(irq);

	return 0;
}
