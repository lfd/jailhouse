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

#ifndef _JAILHOUSE_ASM_SPINLOCK_H
#define _JAILHOUSE_ASM_SPINLOCK_H

#define	RISCV_USE_LR_SC	1

typedef struct {
	int unsigned spin; /* has to have offset 0 */
} spinlock_t;

static inline void spin_init(spinlock_t *lock)
{
	lock->spin = 0;
}

static inline void spin_unlock(spinlock_t *lock)
{
	__asm__ ("\n\
	.if	%[use_lr_sc]\n\
	fence	rw, w\n\
	sw	zero, %[spin]\n\
	.else\n\
	amoswap.w.rl	x0, x0, %[spin]\n\
	.endif\n"
	: [spin] "=&m" (lock->spin):
	[use_lr_sc] "n" (RISCV_USE_LR_SC):
	"memory");
}

static inline void spin_lock(spinlock_t *lock)
{
	/* test and test and set */
	__asm__ ("\n\
	.if	%[use_lr_sc]\n\
\n\
	la	t2, 1\n\
\n\
1:	lw	t1, %[spin]\n\
	bnez	t1, 1b\n\
\n\
2:	lr.w.aq	t1, %[spin]\n\
	bnez	t1, 1b\n\
	sc.w	t1, t2, %[spin]\n\
	bnez	t1, 1b\n\
\n\
	.else\n\
\n\
	# see figure 8.2 in the RISC-V Unprivileged ISA\n\
	li	t2, 1\n\
\n\
3:	lw	t1, %[spin]\n\
	bnez	t1, 3b\n\
\n\
	amoswap.w.aq	t1, t2, %[spin]\n\
	bnez	t1, 3b\n\
\n\
	.endif\n\
"	:
	[spin] "=&m" (lock->spin):
	[use_lr_sc] "n" (RISCV_USE_LR_SC):
	"t1", "t2", "memory");
}

#endif /* !_JAILHOUSE_ASM_SPINLOCK_H */
