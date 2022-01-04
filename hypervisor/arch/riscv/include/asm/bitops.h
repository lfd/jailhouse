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

#ifndef _JAILHOUSE_ASM_BITOPS_H
#define _JAILHOUSE_ASM_BITOPS_H

/* This routine is shared with arm-common */
static inline __attribute__((always_inline)) int
test_bit(unsigned int nr, const volatile unsigned long *addr)
{
	return ((1UL << (nr % BITS_PER_LONG)) &
		(addr[nr / BITS_PER_LONG])) != 0;
}

static inline long unsigned atomic_test_and_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long res, mask;

	mask = 1UL << (nr % BITS_PER_LONG);
	asm volatile("amoor.w.aqrl %0, %2, %1"
		     : "=r"(res), "+A"(addr[nr / BITS_PER_LONG])
		     : "r" (mask)
		     : "memory");

	return (res & mask) != 0;
}

/*
 * Note: the RISC-V Bit Manipulation standard extension will undoubtedly have
 * more performant implementations of these routines Returns the position of
 * the least significant 1, MSB=31, LSB=0.
 * CAUTION: in contrast to POSIX ffs(), LSB = 0
 */
static inline unsigned long ffsl(unsigned long word)
{
	register int result;

	if (!word)
		return BITS_PER_LONG;
	result = BITS_PER_LONG - 1;

	word = (word - 1) ^ word;
	/*
	 * word now contains 0s followed by n 1s, if
	 * n - 1 was the least significant set bit
	 */

	while (0 <= (long) word) {
		word <<= 1;
		--result;
	}
	return result;
}

/*
 * Returns the position of the least significant 0, MSB=31, LSB=0
 * CAUTION: in contrast to the POSIX ffs(), LSB = 0
 */
static inline unsigned long ffzl (register unsigned long word)
{
	return ffsl(~word);
}

#endif /* !_JAILHOUSE_ASM_BITOPS_H */
