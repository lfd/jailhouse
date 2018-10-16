/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Authors:
 *  Lokesh Vutla <lokeshvutla@ti.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define SMCCC_VERSION			0x80000000
#define SMCCC_ARCH_FEATURES		0x80000001

#define ARM_SMCCC_OWNER_MASK		BIT_MASK(29, 24)
#define ARM_SMCCC_OWNER_SHIFT		24

#define ARM_SMCCC_OWNER_ARCH		0
#define ARM_SMCCC_OWNER_SIP             2
#define ARM_SMCCC_OWNER_STANDARD        4

#define ARM_SMCCC_CONV_32		0
#define ARM_SMCCC_CONV_64		1

#define ARM_SMCCC_NOT_SUPPORTED         (-1)
#define ARM_SMCCC_SUCCESS               0

#define ARM_SMCCC_VERSION_1_1		0x10001

#define SMCCC_GET_OWNER(id)	((id & ARM_SMCCC_OWNER_MASK) >>	\
				 ARM_SMCCC_OWNER_SHIFT)

#define SMCCC_IS_CONV_64(function_id)	!!(function_id & (1 << 30))

static inline int smc(unsigned long id)
{
	register unsigned long __id asm("r0") = id;

	asm volatile ("smc #0\n\t"
		: "=r" (__id)
		: "r"(__id)
		: "memory", "x1", "x2", "x3");

	return __id;
}

static inline int smc_arg1(unsigned long id, unsigned long par1)
{
	register unsigned long __id asm("r0") = id;
	register unsigned long __par1 asm("r1") = par1;

	asm volatile ("smc #0\n\t"
		: "=r" (__id)
		: "r"(__id), "r"(__par1)
		: "memory", "x2", "x3");

	return __id;
}

struct trap_context;

int handle_smc(struct trap_context *ctx);
