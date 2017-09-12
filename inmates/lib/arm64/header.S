/*
 * Jailhouse AArch64 support
 *
 * Copyright (C) 2015 Huawei Technologies Duesseldorf GmbH
 *
 * Authors:
 *  Antonios Motakis <antonios.motakis@huawei.com>
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

#include <mach.h>

.macro	ventry	label
	.align	7
	b	\label
.endm

/* Sets up the stack for a CPU. Clobbers x0-x2. Returns 0 in x0.
 *
 * The stack for the n'th onlined CPU is calculated as follows:
 * stack_top = stack_bottom + n * stack_size + stack_size =
 *           = stack_bottom + (n + 1) * stack_size
 *
 * So the stack configuration looks as follows:
 * +-----------+ <- stack_bottom
 * |   Pri     |
 * +-----------+ <- Primary CPU stack top
 * |   Sec1    |
 * +-----------+ <- 1st secondary CPU stack top
 * |   Sec2    |
 * +-----------+ <- 2nd secondary CPU stack top
 * |   ...     |
 */
.macro setup_cpu_stack
	/* increment cpus_online and store */
	mov	x1, #0
	ldr	x0, =cpus_online
	ldr	w1, [x0]
	add	x1, x1, #1
	str	w1, [x0]

	ldr	x0, =stack_bottom

	/* load the stack size */
	mov	x2, #0
	ldr	w2, stack_size

	/* calculate address of CPU stack */
	mul	x1, x1, x2
	add	x0, x0, x1

	mov	sp, x0

	/* return NULL */
	mov	x0, #0
.endm

	.section ".boot", "ax"
	.globl __reset_entry
__reset_entry:
	ldr	x0, =vectors
	msr	vbar_el1, x0

	mov	x0, #(3 << 20)
	msr	cpacr_el1, x0

	setup_cpu_stack
	msr	daif, xzr

	isb

	b	inmate_main

stack_size:
	.word	STACK_SIZE

handle_irq:
	bl	vector_irq
	eret

.weak vector_irq
	b	.

	.globl vectors
	.align 11
vectors:
	ventry	.
	ventry	.
	ventry	.
	ventry	.

	ventry	.
	ventry	handle_irq
	ventry	.
	ventry	.

	ventry	.
	ventry	handle_irq
	ventry	.
	ventry	.

	ventry	.
	ventry	.
	ventry	.
	ventry	.

	.ltorg
