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
#include <jailhouse/printk.h>
#include <asm/psci.h>
#include <asm/traps.h>
#include <asm/smc.h>
#include <asm/smccc.h>

void smccc_discover(void)
{
	int ret;

	ret = smc(PSCI_0_2_FN_VERSION);

	/* We need >=PSCIv1.0 for SMCCC */
	if (PSCI_VERSION_MAJOR(ret) < 1)
		return;

	/* Check if PSCI supports SMCCC version call */
	ret = smc_arg1(PSCI_1_0_FN_FEATURES, SMCCC_VERSION);
	if (ret != ARM_SMCCC_SUCCESS)
		return;

	/* We need to have SMCCC v1.1 */
	ret = smc(SMCCC_VERSION);
	if (ret != ARM_SMCCC_VERSION_1_1)
		return;

	/* check if SMCCC_ARCH_FEATURES is actually available */
	ret = smc_arg1(SMCCC_ARCH_FEATURES, SMCCC_ARCH_FEATURES);
	if (ret != ARM_SMCCC_SUCCESS)
		return;

	ret = smc_arg1(SMCCC_ARCH_FEATURES, SMCCC_ARCH_WORKAROUND_1);

	this_cpu_data()->smccc_has_workaround_1 = ret >= ARM_SMCCC_SUCCESS;
}

static inline long handle_arch_features(u32 id)
{
	switch (id) {
	case SMCCC_ARCH_FEATURES:
		return ARM_SMCCC_SUCCESS;

	case SMCCC_ARCH_WORKAROUND_1:
		return this_cpu_data()->smccc_has_workaround_1 ?
			ARM_SMCCC_SUCCESS : ARM_SMCCC_NOT_SUPPORTED;

	default:
		return ARM_SMCCC_NOT_SUPPORTED;
	}
}

static long handle_arch(struct trap_context *ctx)
{
	u32 function_id = ctx->regs[0];

	switch (function_id) {
	case SMCCC_VERSION:
		return ARM_SMCCC_VERSION_1_1;

	case SMCCC_ARCH_FEATURES:
		return handle_arch_features(ctx->regs[1]);

	default:
		return ARM_SMCCC_NOT_SUPPORTED;
	}
}

enum trap_return handle_smc(struct trap_context *ctx)
{
	unsigned long *regs = ctx->regs;
	u32 *stats = this_cpu_public()->stats;

	switch (SMCCC_GET_OWNER(regs[0])) {
	case ARM_SMCCC_OWNER_ARCH:
		stats[JAILHOUSE_CPU_STAT_VMEXITS_SMCCC]++;
		regs[0] = handle_arch(ctx);
		break;

	case ARM_SMCCC_OWNER_SIP:
		stats[JAILHOUSE_CPU_STAT_VMEXITS_SMCCC]++;
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
