/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 * Copyright (c) Siemens AG, 2016
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/sysregs.h>
	.arm

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

	.globl __reset_entry
__reset_entry:
	ldr	r0, =vectors
	arm_write_sysreg(VBAR, r0)

	mov	r0, #0
	ldr	r1, =bss_start
	ldr	r2, =bss_dwords
	cmp	r2, #0
	beq	2f

1:	str	r0, [r1]
	add	r1, #4
	subs	r2, #1
	bne	1b

2:	ldr	sp, =stack_top

	b	inmate_main

	.globl secondary_startup
secondary_startup:
	ldr	r2, =__c_entry
	ldr	r1, =__cpu_stack_top
	mov	sp, r1
	mov	r0, #0
	str	r0, [r1]
	arm_read_sysreg(MPIDR_EL1, r9)
	and	r0, r9, #MPIDR_CPUID_MASK
	ldr	pc, [r2]

	.ltorg
