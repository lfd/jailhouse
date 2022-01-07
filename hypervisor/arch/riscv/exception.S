/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/asm-defines.h>

.macro  load_csr        csr, temp, value
	li      \temp, \value
	csrw    \csr, \temp
.endm

.macro context_save_all cmd
	.irp reg 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, \
		 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
		\cmd    x\reg, 8*(\reg)(sp)
	.endr
.endm

/*
 * This entry point is used when the hypervisor has been entered already. The
 * existing stack is reused.
 */
.align 4
.globl nested_exception_handler
nested_exception_handler:
	/*
	 * We reuse the old S-Mode stack, so make some place for union
	 * registers. We must not acces this_cpu()-related stuff, as this will
	 * point to the old register set. But it's not necessary, as nested
	 * exception only occur in rare situations, and only one of them (hlvx)
	 * is recoverable. Performance isn't an optimisation criteria, as a
	 * nested exception of hlvx will lead to a crash of the cell in any
	 * case.
	 */
	addi	sp, sp, -REGISTERS_SIZE
	context_save_all sd
	mv	a0, sp
	addi	a0, a0, REGISTERS_SIZE
	sd	a0, REG_SP (sp)
	addi	a0, a0, -REGISTERS_SIZE

	jal	arch_handle_fault

	context_save_all	ld
	addi	sp, sp, REGISTERS_SIZE
	sret

/*
 * This entry point is used when the guest is running.  A new stack needs to be
 * established (and is taken from sscratch).
 */
.align 4
.globl exception_handler
exception_handler:
	csrrw	sp, sscratch, sp
	context_save_all sd

	csrr	t1, sscratch
	sd	t1, REG_SP (sp)

	la	a0, nested_exception_handler
	csrw	stvec, a0

	mv	a0, sp
	jal	arch_handle_trap

.globl vmreturn
vmreturn:
	la	a0, exception_handler
	csrw	stvec, a0

	context_save_all ld
	csrrw	sp, sscratch, sp
	sret

.p2align	12
.globl riscv_park_loop
riscv_park_loop:
	wfi
	j riscv_park_loop
.p2align 12
