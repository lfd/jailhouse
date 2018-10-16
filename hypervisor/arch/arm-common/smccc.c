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

#include <jailhouse/control.h>
#include <asm/psci.h>
#include <asm/traps.h>
#include <asm/smccc.h>

static bool has_workaround_1;
static bool has_workaround_2;

void smccc_init(void)
{
	int ret;

	ret = smc(SMCCC_VERSION);
	if (ret != ARM_SMCCC_VERSION_1_1)
		return;

	/* check if SMCCC_ARCH_FEATURES is actually available */
	ret = smc_arg1(SMCCC_ARCH_FEATURES, SMCCC_ARCH_FEATURES);
	if (ret != ARM_SMCCC_SUCCESS)
		return;

	ret = smc_arg1(SMCCC_ARCH_FEATURES, SMCCC_ARCH_WORKAROUND_1);
	has_workaround_1 = ret >= ARM_SMCCC_SUCCESS;

	ret = smc_arg1(SMCCC_ARCH_FEATURES, SMCCC_ARCH_WORKAROUND_2);
	has_workaround_2 = ret >= ARM_SMCCC_SUCCESS;
}

static long handle_arch(struct trap_context *ctx)
{
	u32 function_id = ctx->regs[0];

	switch (function_id) {
	case SMCCC_VERSION:
		return ARM_SMCCC_VERSION_1_1;

	/* No features supported yet */
	case SMCCC_ARCH_FEATURES:
	default:
		return ARM_SMCCC_NOT_SUPPORTED;
	}
}

int handle_smc(struct trap_context *ctx)
{
	unsigned long *regs = ctx->regs;

	switch (SMCCC_GET_OWNER(regs[0])) {
	case ARM_SMCCC_OWNER_ARCH:
		regs[0] = handle_arch(ctx);
		break;

	case ARM_SMCCC_OWNER_SIP:
		regs[0] = ARM_SMCCC_NOT_SUPPORTED;
		break;

	case ARM_SMCCC_OWNER_STANDARD:
		regs[0] = psci_dispatch(ctx);
		break;

	default:
		return TRAP_UNHANDLED;

	}

	arch_skip_instruction(ctx);

	return TRAP_HANDLED;
}
