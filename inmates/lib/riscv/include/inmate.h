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

#ifndef _JAILHOUSE_INMATE_H
#define _JAILHOUSE_INMATE_H

#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

#define COMM_REGION_BASE	0x100000

#define PAGE_SIZE		(4 * 1024ULL)

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#define SR_SIE	0x00000002UL

#define IRQ_S_TIMER	5
#define IE_TIE		(0x1UL << IRQ_S_TIMER)

#define csr_read(csr)                                           \
({                                                              \
	register unsigned long __v;                             \
	__asm__ __volatile__ ("csrr %0, " __stringify(csr)      \
			      : "=r" (__v) :                    \
			      : "memory");                      \
	__v;                                                    \
})

#define csr_write(csr, val)                                     \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrw " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#define csr_clear(csr, val)                                     \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrc " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#define csr_set(csr, val)                                       \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrs " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

static inline unsigned long get_cycles(void)
{
	return csr_read(time);
}

static inline void disable_irqs(void)
{
	csr_clear(sstatus, SR_SIE);
}

static inline void enable_irqs(void)
{
	csr_set(sstatus, SR_SIE);
}

static inline void timer_enable(void)
{
	csr_set(sie, IE_TIE);
}

static inline void timer_disable(void)
{
	csr_clear(sie, IE_TIE);
}

static inline void cpu_relax(void)
{
	int dummy;

	asm volatile ("div %0, %0, zero" : "=r"(dummy));
	asm volatile ("" ::: "memory");
}

static inline void __attribute__((noreturn)) halt(void)
{
	while (1)
		asm volatile ("wfi" : : : "memory");
}

static inline u8 mmio_read8(void *address)
{
	return *(volatile u8 *)address;
}

static inline u16 mmio_read16(void *address)
{
	return *(volatile u16 *)address;
}

static inline u32 mmio_read32(void *address)
{
	return *(volatile u32 *)address;
}

static inline u64 mmio_read64(void *address)
{
	return *(volatile u64 *)address;
}

static inline void mmio_write8(void *address, u8 value)
{
	*(volatile u8 *)address = value;
}

static inline void mmio_write16(void *address, u16 value)
{
	*(volatile u16 *)address = value;
}

static inline void mmio_write32(void *address, u32 value)
{
	*(volatile u32 *)address = value;
}

static inline void mmio_write64(void *address, u64 value)
{
	*(volatile u64 *)address = value;
}

unsigned long timer_handler(void);

#include <inmate_common.h>

#endif /* !_JAILHOUSE_INMATE_H */
