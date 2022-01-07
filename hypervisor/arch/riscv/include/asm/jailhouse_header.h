/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (C) Siemens AG, 2020
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/*
 * RISC-V defined architectural constraints on the address space:
 * Address immediates are sign extended.
 *
 * lui+jalr can jump to absolute addresses +-2 GiB around zero
 *  --> this best left free for millicode routines (or boot ROM),
 *      at least on 64-bit machines.
 *      This is the range 0xffff_ffff_8000_000--0x0000_0000_7fff_ffff
 *
 * Legal virtual addresses are sign extensions of the most-significant bit
 * mapped by the virtual addressing mode; e.g., in Sv39, bits 63:39 of an
 * address must be a copy of bit 38. Hence, the lowest (signed) address
 * available is 0xffff_ffc0_0000_0000.
 *
 * Linux normally maps itself at 0xffff_ffe0_0000_0000 and modules at
 * 0xffff_ffd0_0000_0000.
 */

/* this provides 16 MB of space for the Jailhouse core */
#define JAILHOUSE_BASE			0xffffffdfff000000
