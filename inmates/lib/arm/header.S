/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 * Copyright (c) OTH Regensburg, 2017
 * Copyright (c) Siemens AG, 2016
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
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

#include <mach.h>
#include <asm/sysregs.h>

	.arm
	.arch_extension virt

	.section ".boot", "ax"
	.align 5
vectors:
	b	__reset_entry
	b	vector_undef
	b	vector_svc
	b	vector_pabt
	b	vector_dabt
	b	vector_unused
	b	vector_irq
	b	vector_fiq

.macro vector, name
	.weak vector_\name
	vector_\name:
	b	.
.endm
	vector undef
	vector svc
	vector pabt
	vector dabt
	vector unused
	vector irq
	vector fiq

/* Sets up the stack for a CPU. Clobbers r0-r2. Returns the location of the IRQ
 * stack in r0.
 *
 * The stack for the n'th onlined CPU is calculated as follows:
 * stack_top = stack_bottom + (n * stack_size * 2) + stack_size =
 *           = stack_bottom + (2 * n + 1) * stack_size
 *
 * So the stack configuration looks as follows:
 * +-----------+ <- stack_bottom
 * |   Pri     |
 * +-----------+ <- Primary CPU stack top
 * | Pri_IRQ0  |
 * +-----------+ <- Primary CPU IRQ stack top
 * |   Sec1    |
 * +-----------+ <- 1st secondary CPU stack top
 * | Sec_IRQ1  |
 * +-----------+ <- 1st secondary CPU IRQ stack top
 * |   Sec2    |
 * +-----------+ <- 2nd secondary CPU stack top
 * |   ...     |
 */
.macro setup_cpu_stack
	/* increment cpus_online and store */
	ldr	r0, =cpus_online
	ldr	r1, [r0]
	add	r1, r1, #1
	str	r1, [r0]
	sub	r1, r1, #1

	ldr	r0, =__stack_bottom

	/* load the stack size */
	ldr	r2, stack_size

	/* calculate address of CPU stack */
	mov	r1, r1, lsl #1
	add	r1, r1, #1
	mul	r1, r1, r2
	add	r0, r0, r1

	mov	sp, r0

	/* return location of the IRQ stack */
	add	r0, r0, r2
.endm

.macro load_vectors, reg
	ldr	\reg, =vectors
	arm_write_sysreg(VBAR, \reg)
.endm

	.globl __reset_entry
__reset_entry:
	load_vectors r0

	mov	r0, #0
	ldr	r1, =bss_start
	ldr	r2, =bss_dwords
	cmp	r2, #0
	beq	2f

1:	str	r0, [r1]
	add	r1, #4
	subs	r2, #1
	bne	1b

2:	setup_cpu_stack

	b	inmate_main

stack_size:
	.word STACK_SIZE

	.globl secondary_startup
secondary_startup:
	/* prepare vector base address */
	load_vectors r0

	/* prepare stack */
	setup_cpu_stack

	ldr	r1, =__c_entry
	ldr	pc, [r1]

	.ltorg
