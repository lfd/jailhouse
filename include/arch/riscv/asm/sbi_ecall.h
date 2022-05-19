/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022
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

/* see https://github.com/riscv/riscv-sbi-doc/blob/master/riscv-sbi.adoc */

#ifndef _SBI_ECALL_H
#define _SBI_ECALL_H

#define SBI_SUCCESS		0
#define SBI_ERR_FAILED		-1
#define SBI_ERR_NOT_SUPPORTED	-2
#define SBI_ERR_INVALID_PARAM	-3
#define SBI_ERR_DENIED		-4
#define SBI_ERR_INVALID_ADDRESS	-5
#define SBI_ERR_ALREADY_AVAILABLE	-6

#define SBI_EXT_TIME		0x54494D45
#define SBI_EXT_TIME_SET_TIMER	0x0

#ifndef __ASSEMBLY__

struct sbiret {
	long error;
	long value;
};

static inline struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
				      unsigned long arg1, unsigned long arg2,
				      unsigned long arg3, unsigned long arg4,
				      unsigned long arg5)
{
	struct sbiret ret;

	register unsigned long a0 asm ("a0") = arg0;
	register unsigned long a1 asm ("a1") = arg1;
	register unsigned long a2 asm ("a2") = arg2;
	register unsigned long a3 asm ("a3") = arg3;
	register unsigned long a4 asm ("a4") = arg4;
	register unsigned long a5 asm ("a5") = arg5;
	register unsigned long a6 asm ("a6") = fid;
	register unsigned long a7 asm ("a7") = ext;
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4),
		        "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

static inline struct sbiret sbi_set_timer(unsigned long stime_value)
{
	return sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, stime_value,
			 0, 0, 0, 0, 0);
}

#endif /* !__ASSEMBLY__ */

#endif /* _SBI_ECALL_H */
