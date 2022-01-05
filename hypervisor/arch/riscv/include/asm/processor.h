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

#ifndef _JAILHOUSE_ASM_PROCESSOR_H
#define _JAILHOUSE_ASM_PROCESSOR_H

#include <jailhouse/types.h>

union registers {
	struct {
		unsigned long pc; /* x0 is zero register, in our case it's PC */
		unsigned long ra;
		unsigned long sp;
		unsigned long gp;
		unsigned long tp;
		unsigned long t0;
		unsigned long t1;
		unsigned long t2;
		unsigned long s0;
		unsigned long s1;
		unsigned long a0;
		unsigned long a1;
		unsigned long a2;
		unsigned long a3;
		unsigned long a4;
		unsigned long a5;
		unsigned long a6;
		unsigned long a7;
		unsigned long s2;
		unsigned long s3;
		unsigned long s4;
		unsigned long s5;
		unsigned long s6;
		unsigned long s7;
		unsigned long s8;
		unsigned long s9;
		unsigned long s10;
		unsigned long s11;
		unsigned long t3;
		unsigned long t4;
		unsigned long t5;
		unsigned long t6;
	};
	struct riscv_raw_registers {
		unsigned long x[32];
	} raw;
} __attribute__ ((__aligned__ (1 << (7 - 3) /* bits/byte */)));
/* RISC-V ABI requires 128-bit stack alignment */

static inline void cpu_relax(void)
{
	int dummy;

	/* In lieu of a halt instruction, induce a long-latency stall. */
	asm volatile("div %0, %0, zero" : "=r" (dummy));
}

static inline void memory_barrier(void)
{
	asm volatile("fence rw, rw" : : : "memory");
}

static inline void memory_load_barrier(void)
{
	asm volatile("fence r, rw" : : : "memory");
}

#endif /* !_JAILHOUSE_ASM_PROCESSOR_H */
