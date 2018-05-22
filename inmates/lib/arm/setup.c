#include <inmate.h>
#include <mach.h>
#include <asm/sysregs.h>

#define HUGE_PAGE_SIZE		(2 * 1024 * 1024ULL)
#define HUGE_PAGE_MASK		(~(HUGE_PAGE_SIZE - 1))

#define LONG_DESC_BLOCK 0x1
#define LONG_DESC_TABLE 0x3

#define ATTR_MAIR(n)		(((n) & 0x3) << 2)
#define ATTR_AP(n)		(((n) & 0x3) << 6)

#define PERM_RW_EL1	ATTR_AP(0)
#define PERM_RW_ANY	ATTR_AP(1)
#define PERM_RO_EL1	ATTR_AP(2)
#define PERM_RO_ANY	ATTR_AP(3)

#define ATTR_INNER_SHAREABLE	(3 << 8)
#define ATTR_AF			(1 << 10)
#define ATTR_CONT		(1 << 12)

#define PGD_INDEX(addr)		((addr) >> 30)
#define PMD_INDEX(addr)		(((addr) >> 21) & 0x1ff)

extern unsigned long __inmate_base[];

static u64 __attribute__((aligned(4096))) page_directory[4];

void map_range(void *start, unsigned long size, enum map_type map_type)
{
	unsigned int vaddr = (unsigned int)start;
	unsigned pgd_index;
	u64 pmd_entry;
	u64 *pmd;

	size += (vaddr & ~HUGE_PAGE_MASK) + HUGE_PAGE_SIZE - 1;
	size &= HUGE_PAGE_MASK;

	while (size) {
		pgd_index = PGD_INDEX(vaddr);
		if (!(page_directory[pgd_index] & LONG_DESC_TABLE)) {
			pmd = alloc(PAGE_SIZE, PAGE_SIZE);
			memset(pmd, 0, PAGE_SIZE);
			page_directory[pgd_index] = (unsigned int)pmd | LONG_DESC_TABLE;
		} else {
			pmd = (u64*)(unsigned)(page_directory[pgd_index] & ~LONG_DESC_TABLE);
		}

		pmd_entry = vaddr & HUGE_PAGE_MASK;
		pmd_entry |= ATTR_AF | ATTR_INNER_SHAREABLE | PERM_RW_ANY | LONG_DESC_BLOCK;
		pmd_entry |= (map_type == MAP_CACHED) ? ATTR_MAIR(0) : ATTR_MAIR(1);

		pmd[PMD_INDEX(vaddr)] = pmd_entry;

		size -= HUGE_PAGE_SIZE;
		vaddr += HUGE_PAGE_SIZE;
	}
}

void arch_init_early(void)
{
	u32 ttbcr, sctlr;

	map_range((void*)__inmate_base, 0x10000, MAP_CACHED);
	map_range((void*)COMM_REGION_BASE, PAGE_SIZE, MAP_CACHED);
	map_range(GICC_V2_BASE, PAGE_SIZE, MAP_UNCACHED);
	map_range(GICD_V2_BASE, PAGE_SIZE, MAP_UNCACHED);
	if (CON_BASE)
		map_range((void*)CON_BASE, PAGE_SIZE, MAP_UNCACHED);

	/*
	 * MAIR attributes 0: inner/outer: normal memory, outer write-back
	 * non-transient
	 * MAIR other attributes: device memory
	 */
	arm_write_sysreg(MAIR0, MAIR_ATTR(0, MAIR_ATTR_WBRWA));
	arm_write_sysreg(MAIR1, 0x0);

	arm_read_sysreg(TTBCR, ttbcr);
	/*
	 * Enable extended address enable and set inner/outer caches to
	 * write-back write-allocate cacheable and shareability attribute to
	 * inner shareable
	 */
	ttbcr = TTBCR_EAE | TTBCR_IRGN0_WB_WA | TTBCR_ORGN0_WB_WA | \
		TTBCR_SH0_INNER_SHAREABLE | 0;
	arm_write_sysreg(TTBCR, ttbcr);

	/* We only have one first level PT in page_table */
	arm_write_sysreg(TTBR0, page_directory);

	arm_read_sysreg(SCTLR_EL1, sctlr);
	/* Disable TEX remap functionality */
	sctlr &= ~SCTLR_TRE_BIT;
	/* Enable MMU, round-robin replacement, data+instruction caches */
	sctlr |= SCTLR_M_BIT | SCTLR_RR_BIT | SCTLR_C_BIT | SCTLR_I_BIT;
	arm_write_sysreg(SCTLR_EL1, sctlr);
	/* MMU is enabled from now on */
}
