/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/gen-defines.h>
#include <jailhouse/percpu.h>

void common(void);

void common(void)
{
	OFFSETU(REG_SP, registers, sp);
	OFFSETU(REG_RA, registers, ra);
	OFFSETU(REG_T0, registers, t0);
	OFFSETU(REG_TP, registers, tp);
	OFFSETU(REG_GP, registers, gp);
	OFFSETU(REG_S0, registers, s0);
	OFFSETU(REG_S1, registers, s1);
	OFFSETU(REG_S2, registers, s2);
	OFFSETU(REG_S3, registers, s3);
	OFFSETU(REG_S4, registers, s4);
	OFFSETU(REG_S5, registers, s5);
	OFFSETU(REG_S6, registers, s6);
	OFFSETU(REG_S7, registers, s7);
	OFFSETU(REG_S8, registers, s8);
	OFFSETU(REG_S9, registers, s9);
	OFFSETU(REG_S10, registers, s10);
	OFFSETU(REG_S11, registers, s11);

	OFFSET(HEADER_MAX_CPUS, jailhouse_header, max_cpus);
	OFFSET(HEADER_PERCPU_SIZE, jailhouse_header, percpu_size);

	OFFSET(CFG_PHYS_START, jailhouse_system, hypervisor_memory.phys_start);

	OFFSET(PCPU_GUEST_REGS, per_cpu, guest_regs);

	DEFINE(REGISTERS_SIZE, sizeof(union registers));
}
