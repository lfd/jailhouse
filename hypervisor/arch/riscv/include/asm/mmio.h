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
 */

/*
 * We need those definitions, as I saw the compiler emitting 4x 1B loads in
 * case of mmio_read32.
 */

#define DEFINE_MMIO_READ
#define DEFINE_MMIO_WRITE

static inline void mmio_write8(volatile void *addr, u8 val)
{
	asm volatile("sb %0, 0(%1)" : : "r" (val), "r" (addr));
}

static inline void mmio_write16(volatile void *addr, u16 val)
{
	asm volatile("sh %0, 0(%1)" : : "r" (val), "r" (addr));
}

static inline void mmio_write32(volatile void *addr, u32 val)
{
	asm volatile("sw %0, 0(%1)" : : "r" (val), "r" (addr));
}

static inline void mmio_write64(volatile void *addr, u64 val)
{
	asm volatile("sd %0, 0(%1)" : : "r" (val), "r" (addr));
}

static inline u8 mmio_read8(const volatile void *addr)
{
	u8 val;
	asm volatile("lb %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}

static inline u16 mmio_read16(const volatile void *addr)
{
	u16 val;
	asm volatile("lh %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}

static inline u32 mmio_read32(const volatile void *addr)
{
	u32 val;
	asm volatile("lw %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}

static inline u64 mmio_read64(const volatile void *addr)
{
	u64 val;
	asm volatile("ld %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}
