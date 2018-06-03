/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Ralf Ramsauer, 2018
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

#include <types.h>

static inline void outb(u8 v, u16 port)
{
	asm volatile("outb %0,%1" : : "a" (v), "dN" (port));
}

static inline void outw(u16 v, u16 port)
{
	asm volatile("outw %0,%1" : : "a" (v), "dN" (port));
}

static inline void outl(u32 v, u16 port)
{
	asm volatile("outl %0,%1" : : "a" (v), "dN" (port));
}

static inline u8 inb(u16 port)
{
	u8 v;
	asm volatile("inb %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline u16 inw(u16 port)
{
	u16 v;
	asm volatile("inw %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline u32 inl(u16 port)
{
	u32 v;
	asm volatile("inl %1,%0" : "=a" (v) : "dN" (port));
	return v;
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
	u32 value;

	/* assembly-encoded to match the hypervisor MMIO parser support */
	asm volatile("movl (%1),%0" : "=r" (value) : "r" (address));
	return value;
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
	/* assembly-encoded to match the hypervisor MMIO parser support */
	asm volatile("movl %0,(%1)" : : "r" (value), "r" (address));
}

static inline void mmio_write64(void *address, u64 value)
{
	*(volatile u64 *)address = value;
}
