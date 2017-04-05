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

/* The following definitions are inspired by
 * hypervisor/arch/arm64/include/asm/sysregs.h */

#define MPIDR_CPUID_MASK    0xff00ffffff

#ifndef __ASSEMBLY__

#define ICC_SGI1R_EL1                    SYSREG_64(0, c12, c11, 5)

#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)

#define SYSREG_32(op1, crn, crm, op2)	s3_##op1 ##_##crn ##_##crm ##_##op2
#define SYSREG_64(op1, crn, crm, op2)       SYSREG_32(op1, crn, crm, op2)

#define arm_write_sysreg(sysreg, val) \
	asm volatile ("msr	"__stringify(sysreg)", %0\n" : : "r"((u64)(val)))

#define arm_read_sysreg(sysreg, val) \
	asm volatile ("mrs	%0,  "__stringify(sysreg)"\n" : "=r"((val)))

#endif /* __ASSEMBLY__ */
