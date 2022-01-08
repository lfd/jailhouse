/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sbi_ecall.h"

/* CPU statistics, RISC-V-specific part */
#define JAILHOUSE_CPU_STAT_VMEXITS_SBI_IPI	JAILHOUSE_GENERIC_CPU_STATS
#define JAILHOUSE_CPU_STAT_VMEXITS_SBI_RFENCE	JAILHOUSE_GENERIC_CPU_STATS + 1
#define JAILHOUSE_CPU_STAT_VMEXITS_SBI_TIME	JAILHOUSE_GENERIC_CPU_STATS + 2
#define JAILHOUSE_CPU_STAT_VMEXITS_SBI_OTHER	JAILHOUSE_GENERIC_CPU_STATS + 3
#define JAILHOUSE_CPU_STAT_VMEXITS_TIMER	JAILHOUSE_GENERIC_CPU_STATS + 4
#define JAILHOUSE_CPU_STAT_VMEXITS_IPI		JAILHOUSE_GENERIC_CPU_STATS + 5
#define JAILHOUSE_CPU_STAT_VMEXITS_VIRQ		JAILHOUSE_GENERIC_CPU_STATS + 6
#define JAILHOUSE_NUM_CPU_STATS			JAILHOUSE_GENERIC_CPU_STATS + 7

#define ASCII32(C, POS)	(C << ((POS) * 8))
#define JAILHOUSE_EID		\
	ASCII32('J', 3) |	\
	ASCII32('H', 2) |	\
	ASCII32('O', 1) |	\
	ASCII32('U', 0)

#ifndef __ASSEMBLY__

struct jailhouse_comm_region {
	COMM_REGION_GENERIC_HEADER;

	__u64 timebase_frequency;
} __attribute__((packed));

static inline unsigned long jailhouse_call(unsigned long num)
{
	struct sbiret ret;

	ret = sbi_ecall(JAILHOUSE_EID, num, 0, 0, 0, 0, 0, 0);

	return ret.error;
}

static inline unsigned long
jailhouse_call_arg1(unsigned long num, unsigned long arg1)
{
	struct sbiret ret;

	ret = sbi_ecall(JAILHOUSE_EID, num, arg1, 0, 0, 0, 0, 0);

	return ret.error;
}

static inline unsigned long
jailhouse_call_arg2(unsigned long num, unsigned long arg1, unsigned long arg2)
{
	struct sbiret ret;

	ret = sbi_ecall(JAILHOUSE_EID, num, arg1, arg2, 0, 0, 0, 0);

	return ret.error;
}

static inline void
jailhouse_send_msg_to_cell(struct jailhouse_comm_region *comm_region,
			   __u32 msg)
{
	comm_region->reply_from_cell = JAILHOUSE_MSG_NONE;
	/* ensure reply was cleared before sending new message */
	asm volatile("fence rw, rw" : : : "memory");
	comm_region->msg_to_cell = msg;
}

static inline void
jailhouse_send_reply_from_cell(struct jailhouse_comm_region *comm_region,
			       __u32 reply)
{
	comm_region->msg_to_cell = JAILHOUSE_MSG_NONE;
	/* ensure message was cleared before sending reply */
	asm volatile("fence rw, rw" : : : "memory");
	comm_region->reply_from_cell = reply;
}

#endif /* !__ASSEMBLY__ */
