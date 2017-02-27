/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2017
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/sysregs.h>

static inline void ccnt_reset(void)
{
	/* enable counters and reset them */
	arm_write_sysreg(PMCR_EL0, 0x5);
}

static inline unsigned int ccnt_read(void)
{
	unsigned int value;
	arm_read_sysreg(PMCCNTR_EL0, value);
	return value;
}

static inline void ccnt_init(void)
{
	/* enable cycle counter */
	arm_write_sysreg(PMCNTENSET_EL0, 0x80000000);
	/* disable interrupts */
	arm_write_sysreg(PMINTENCLR_EL1, 0x80000003);

	ccnt_reset();
}

static inline void ccnt_deinit(void)
{
	/* Disable counters */
	arm_write_sysreg(PMCNTENCLR_EL0, 0x80000000);
}
