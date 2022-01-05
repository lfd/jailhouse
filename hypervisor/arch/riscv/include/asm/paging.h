/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_ASM_PAGING_H
#define _JAILHOUSE_ASM_PAGING_H

#include <jailhouse/types.h>
#include <jailhouse/header.h>
#include <asm/csr64.h>

#define PAGE_SHIFT		12

#define RISCV_PAGE_SV_DEPTH	39
#define RISCV_HV_PAGE_SV_DEPTH	39

/* SV32 requires 2, SV39 3, SV48 4. Once we support SV57, this should be 5. */
#define MAX_PAGE_TABLE_LEVELS	4
#define PAGE_FLAG_FRAMEBUFFER	0

/*
 * RISC-V does not insert caching/prefetching/... information into page tables.
 * Instead, this is done via `Physical Memory Attributes'. However, the ISA
 * does not define a concrete form of PMAs.  E.g., they could be hardcoded by
 * physical address in a SoC.
 */
#define PAGE_FLAG_DEVICE	0

#define RISCV_PAGE_WIDTH	12
#define RISCV_PTE_SIZE		3
#define RISCV_PTES_PER_NODE	(RISCV_PAGE_WIDTH - RISCV_PTE_SIZE)

#define RISCV_PTE_V	0
#define RISCV_PTE_R	1
#define RISCV_PTE_W	2
#define RISCV_PTE_X	3
#define RISCV_PTE_U	4
#define RISCV_PTE_G	5
#define RISCV_PTE_A	6
#define RISCV_PTE_D	7

#define RISCV_PTE_FLAG(FLAG)	(1 << RISCV_PTE_ ## FLAG)


#define PAGE_PRESENT_FLAGS		(RISCV_PTE_FLAG(V))
#define PAGE_NONPRESENT_FLAGS	0
#define PAGE_GUEST_PRESENT_FLAGS	(PAGE_PRESENT_FLAGS | RISCV_PTE_FLAG(U))

#define	PAGE_DEFAULT_FLAGS	(PAGE_PRESENT_FLAGS | RISCV_PTE_FLAG(R) | \
				 RISCV_PTE_FLAG(W) | RISCV_PTE_FLAG(X))

#define PAGE_READONLY_FLAGS	(PAGE_PRESENT_FLAGS | RISCV_PTE_FLAG(R))


#define INVALID_PHYS_ADDR	(~0UL)

#define TEMPORARY_MAPPING_BASE	(~0UL << (RISCV_PAGE_SV_DEPTH - 1))
#define NUM_TEMPORARY_PAGES	16

#define NUM_REMAP_BITMAP_PAGES	4
/*
 * REMAP_BASE hast to be described by the same top-level PTE as JAILHOUSE_BASE,
 * otherwise it won't be added to non-CPU 0 page tables automatically
 */
#define REMAP_BASE\
	(JAILHOUSE_BASE & ~0UL << (RISCV_HV_PAGE_SV_DEPTH - 9))

#if REMAP_BASE + (NUM_REMAP_BITMAP_PAGES * 1UL << /*
				*/   12 /* ld bytes/page
				*/ +  3 /* ld bits/byte
				*/ + 12 /* each bit represents a page
				*/) > JAILHOUSE_BASE

# error Overlap between REMAP area and JAILHOUSE_BASE!
#endif

#define CELL_ROOT_PT_PAGES	(1 << 2)

#define ATP_MODE_SHIFT		60
#define ATP_MODE_SV39		0x8
#define ATP_MODE_SV48		0x9
#define ATP_MODE_SV57		0xa

#ifndef __ASSEMBLY__

#include <jailhouse/string.h>

typedef size_t *pt_entry_t;

/* MMU mode for Jailhouse (S-Mode) */
extern unsigned char hv_atp_mode;

static inline void arch_paging_flush_page_tlbs(size_t page_addr)
{
	asm volatile("sfence.vma /* rd, */ zero, %[addr]" :
		     : [addr] "r" (page_addr));
}

/*
 * In RISC-V, the MMU accesses page tables through the caches (the MMU is a
 * coherent agent)
 */
static inline void arch_paging_flush_cpu_caches(void const *addr, size_t size)
{
}

#define ENABLE_MMU(NAME, REG)						\
static inline void enable_mmu_##NAME(u8 mode, unsigned long pt)		\
{									\
	u64 atp;							\
									\
	atp = (u64)mode << ATP_MODE_SHIFT | (u64)pt >> PAGE_SHIFT;	\
	asm volatile("sfence.vma\n"					\
		     "csrw %0, %1\n"					\
		     "sfence.vma\n"					\
		     : : "i"(REG), "rK"(atp) : "memory");		\
}

ENABLE_MMU(satp, CSR_SATP)
ENABLE_MMU(hgatp, CSR_HGATP)

struct paging_structures;
struct cell;

void riscv_paging_vcpu_init(struct paging_structures *pg_structs);
int riscv_paging_cell_init(struct cell *const cell);

#endif /* !__ASSEMBLY__ */

#endif /* !_JAILHOUSE_ASM_PAGING_H */
