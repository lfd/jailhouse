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

/* see https://github.com/riscv/riscv-sbi-doc/blob/master/riscv-sbi.adoc */

#ifndef _SBI_H
#define _SBI_H

#include <asm/sbi_ecall.h>

#define RISCV_SBI_HART_STATE_STARTED		0
#define RISCV_SBI_HART_STATE_STOPPED		1
#define RISCV_SBI_HART_START_REQUEST_PENDING	2
#define RISCV_SBI_HART_STOP_REQUEST_PENDING	3

#define SBI_EXT_BASE			0x10
#define SBI_EXT_BASE_GET_SPEC_VERSION	0
#define SBI_EXT_BASE_GET_IMP_ID		1
#define SBI_EXT_BASE_GET_IMP_VERSION	2
#define SBI_EXT_BASE_PROBE_EXT		3
#define SBI_EXT_BASE_GET_MVENDORID	4
#define SBI_EXT_BASE_GET_MARCHID	5
#define SBI_EXT_BASE_GET_MIMPID		6

#define SBI_EXT_0_1_SET_TIMER		0x0
#define SBI_EXT_0_1_CONSOLE_PUTCHAR	0x1
#define SBI_EXT_0_1_CONSOLE_GETCHAR	0x2

#define SBI_EXT_SPI		0x735049
#define SBI_EXT_IPI_SEND_IPI	0x0

#define SBI_EXT_RFENCE		 	0x52464E43
#define SBI_REMOTE_FENCE_I       	0
#define SBI_REMOTE_SFENCE_VMA		1
#define SBI_REMOTE_SFENCE_VMA_ASID	2
#define SBI_REMOTE_HFENCE_GVMA_VMID	3
#define SBI_REMOTE_HFENCE_GVMA		4
#define SBI_REMOTE_HFENCE_VVMA_ASID	5
#define SBI_REMOTE_HFENCE_VVMA		6

#define SBI_EXT_HSM			0x48534D
#define SBI_EXT_HSM_HART_START		0
#define SBI_EXT_HSM_HART_STOP		1
#define SBI_EXT_HSM_HART_STATUS		2
#define SBI_EXT_HSM_HART_SUSPEND	3

#define SBI_EXT_SRST			0x53525354

#ifndef __ASSEMBLY__

static inline struct sbiret sbi_send_ipi(unsigned long hart_mask,
					 unsigned long hart_mask_base)
{
	return sbi_ecall(SBI_EXT_SPI, SBI_EXT_IPI_SEND_IPI,
			 hart_mask, hart_mask_base, 0, 0, 0, 0);
}

static inline void sbi_console_putchar_legacy0_1(int ch)
{
	sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

static inline struct sbiret sbi_console_getchar_legacy_0_1(void)
{
	return sbi_ecall(SBI_EXT_0_1_CONSOLE_GETCHAR, 0, 0, 0, 0, 0, 0, 0);
}

static inline struct sbiret sbi_hart_stop(void)
{
	return sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STOP, 0, 0, 0, 0, 0, 0);
}

#endif /* !__ASSEMBLY__ */

#endif /* _SBI_H */
