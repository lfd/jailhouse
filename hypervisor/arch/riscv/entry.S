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

#include <asm/asm-defines.h>
#include <asm/csr64.h>
#include <asm/paging.h>

#include <jailhouse/console.h>

#define SSTATUS_INITIAL (SR_SPIE | SR_SPP | SR_FS_DIRTY | \
			 SR_VS_DIRTY | SR_XS_DIRTY | SR_UXL_64)

#define PAGING_FLAGS_NEXT_LEVEL	RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(V)
#define PAGING_FLAGS_BOOTSTRAP	PAGE_DEFAULT_FLAGS | RISCV_PTE_FLAG(G) | \
				RISCV_PTE_FLAG(A) | RISCV_PTE_FLAG(D)

/*
 * hideleg needs to be set early, otherwise vsip/vsie are forced to 0. Bits set
 * here causes aliasing of VSIP/HIP and VSIE/HIE
 */
#define INITIAL_HIDELEG \
	((IE_SIE << VSIP_TO_HVIP_SHIFT) | \
	(IE_TIE << VSIP_TO_HVIP_SHIFT) | \
	(IE_EIE << VSIP_TO_HVIP_SHIFT))

#define INITIAL_SIE (IE_SIE | IE_TIE | IE_EIE)

.macro csr_from_csr vsreg, sreg, t
	csrr	\t, \sreg
	csrw	\vsreg, \t
.endm

.macro load_csr	csr, temp, value
	li	\temp, \value
	csrw	\csr, \temp
.endm

.macro access_config op, reg, offset
	\op	\reg, \offset (a5)
.endm

.macro ld_constant reg, constant
	ld	\reg, %lo (pool_\constant - constant_pool) (t6)
.endm

.macro bootstrap_page macro
	.irp page,\
		bt_tbl_l0,\
		bt_tbl_l1_0,\
		bt_tbl_l2_0,\
		bt_tbl_l3_0,\
		bt_tbl_l1_1,\
		bt_tbl_l2_1,\
		bt_tbl_l3_1,\
		bt_tbl_l1_2,\
		bt_tbl_l2_2,\
		bt_tbl_l3_2
	\macro	\page
	.endr
.endm
.globl bt_tbl_l0

.macro define_page page
	.p2align RISCV_PAGE_WIDTH
	\page:
	.skip (1 << RISCV_PAGE_WIDTH)
.endm

.macro pool_constant symbol
	.p2align 3
	pool_\symbol:
	.8byte	\symbol
.endm

/*
 * level: 0 for 4 KiB leaves
 * virt: virtual address to map
 * pte: pte contents: phys addr for leaves, sub node otherwise; clobbered
 * table: node to write pte to
 * temp: clobberable register
 * flags: PTE flag bits
 */
.macro riscv_pt_write_pte level, virt, pte, table, temp, flags
	ori	\pte, \pte, \flags
	/* RISC-V uses 12-bit _signed_ immediates, so extracting the
	 * least-significant 12 bits can't be done in a single instruction of
	 * the base ABI
	 */
	srli	\temp, \virt,\
			(RISCV_PAGE_WIDTH + (\level) * RISCV_PTES_PER_NODE)
	andi	\temp, \temp, 1 << (RISCV_PAGE_WIDTH - RISCV_PTE_SIZE) - 1
	slli	\temp, \temp, RISCV_PTE_SIZE
	add	\temp, \temp, \table
	sd	\pte, 0 (\temp)
.endm

.macro riscv_pt_write_address level, virt, addr, table, temp, flags
	srli	\addr, \addr, 2
	riscv_pt_write_pte	\level, \virt, \addr, \table, \temp, \flags
.endm

.macro write_table lvl, table, flags, dst
	la	t0, \table

	.ifnb \dst /* No identity map */
	ld_constant	t3, \dst
	add	t4, a2, t3
	.else /* identity map */
	move	t4, t2
	.endif

	riscv_pt_write_address   \lvl, t2, t4, t0, t5, \flags
.endm

.macro sv57_map_2M l0, l1, l2, l3, phys
	write_table	4, \l0, PAGING_FLAGS_NEXT_LEVEL, \l1
	write_table	3, \l1, PAGING_FLAGS_NEXT_LEVEL, \l2
	write_table	2, \l2, PAGING_FLAGS_NEXT_LEVEL, \l3
	write_table	1, \l3, PAGING_FLAGS_BOOTSTRAP, \phys
.endm

.macro sv48_map_2M l0, l1, l2, phys
	write_table	3, \l0, PAGING_FLAGS_NEXT_LEVEL, \l1
	write_table	2, \l1, PAGING_FLAGS_NEXT_LEVEL, \l2
	write_table	1, \l2, PAGING_FLAGS_BOOTSTRAP, \phys
.endm

.macro sv39_map_2M l0, l1, phys
	write_table	2, \l0, PAGING_FLAGS_NEXT_LEVEL, \l1
	write_table	1, \l1, PAGING_FLAGS_BOOTSTRAP, \phys
.endm

/* place large constants into here */
.section .rodata
pool_constant	virtual_arch_entry
bootstrap_page	pool_constant

.p2align 3
constant_pool:
.p2align 3
pool_jailhouse_base:
	.8byte	JAILHOUSE_BASE

.section .data
	bootstrap_page	define_page
	.purgem	define_page

.text
.globl arch_entry
arch_entry:
	/*
	 * a0: cpuid
	 * t5: hypervisor_header
	 * t6: __page_pool
	 */
	la	t5, hypervisor_header
	la	t6, __page_pool

	/*
	 * t1: max_cpus
	 * t2: sizeof(struct per_cpu)
	 * Note: this would also be available directly: sizeof$struct$per_cpu
	 */
	lw		t1, HEADER_MAX_CPUS (t5)
	lw		t2, HEADER_PERCPU_SIZE (t5)

	/* a5: configuration area */
	mul	t2, t2, t1
	add	a5, t2, t6

	/* t6: constant_pool */
	la	t6, constant_pool

	/*
	 * these pages are used during the initialization phase currently, they
	 * are not reclaimed afterwards. Create a "bootstrap" paging table
	 * that maps the the hypervisor image to the logical/physical addresses
	 * it was linked and also where it was currently linked.
	 *
	 * In case of any MMU variant, we always need to map 2M-Pages.
	 * Depending on the MMU variant, this may require different 'n' levels
	 * of page tables.
	 *   SV39: 2 levels
	 *   SV48: 3 levels
	 *   SV57: 4 levels
	 *
	 * We always require one root table, and then (n-1)*2 subtables.
	 *   SV39: 1+1*2 = 3 pages
	 *   SV48: 1+2*2 = 5 pages
	 *   SV57: 1+3*2 = 7 pages
	 *
	 * The problem is the location of those page tables. For the worst
	 * case, SV57, which we need to prepare for, we would need to reserve 7
	 * pages somewhere. If we then land on a SV39 system, we waste 4 pages
	 * (16KiB).
	 *
	 * So for the moment, simply always statically allocate 7 pages that
	 * are dedicated for the bootstrap page tables. We can't reclaim
	 * memory, as we need those bootstrap tables for the rollback
	 * trampoline, when disabling the hypervisor.
	 *
	 * An alternative I could think of is to allocate those pages at the
	 * very end of hypervisor_mem. We could fill pages, and mark those
	 * pages as used during arch_paging_init. This way, we wouldn't waste
	 * any memory, but it's more complex. For the moment, let's implement
	 * the simple variant, and allocate 7 pages.
	 *
	 * Just some further notes:
	 * If Linux chose to select, for example, SV48, the the bootstrap
	 * tables MUST also select SV48. This is, because there's a 99% chance
	 * that Linux chose an address to remap Jailhouse that is only
	 * adressable with a SV48. Hence, the trampolin page tables must also
	 * be at least SV48. Later, when setting up the final tables, we could
	 * switch down to SV39. This is recommendable, as it requires less page
	 * table lookups.
	 *
	 * t0: Lx VA bootstrap_table_l0
	 * t3: JH VA of bootstrap_table_hypervisor_l1
	 */
	ld_constant	t2, jailhouse_base
	/* a3: jailhouse_memory.phys_start */
	access_config	ld, a3, SYSCONFIG_HYPERVISOR_PHYS
	/* a2: virt_to_phys; phys = virt_to_phys + virt */
	sub	a2, a3, t2

	/* Get MMU-Variant that Linux currently uses */
	csrr	a4, satp
	srli	a4, a4, ATP_MODE_SHIFT

	/* Prepare address of debug console in t2, it will be ID-mapped */
	access_config	ld, t2, SYSCONFIG_DEBUG_CONSOLE_PHYS
	la	t0, hypervisor_header
	sd	t2, HEADER_DEBUG_CONSOLE_VIRT (t0)
	/* Align to next 2M-Page */
	li		a6, ~0x1fffff
	and		t2, t2, a6
	/* Only map debug_console if it's not SBI */
	access_config 	lh, t4, SYSCONFIG_DEBUG_CONSOLE_TYPE
	li		t3, JAILHOUSE_CON_TYPE_RISCV_SBI


	/* And choose the same variant for the bootstrap tables */
	li	t0, ATP_MODE_SV39
	beq	a4, t0, sv39
	li	t0, ATP_MODE_SV48
	beq	a4, t0, sv48
	li	t0, ATP_MODE_SV57
	beq	a4, t0, sv57

	/* Unknown MMU type if we reach that */
1:	wfi
	j	1b

	/* virt must be held in t2 */
sv39:
	beq		t4, t3, 1f
	sv39_map_2M	bt_tbl_l0, bt_tbl_l1_2
1:
	ld_constant	t2, jailhouse_base
	sv39_map_2M	bt_tbl_l0, bt_tbl_l1_0, jailhouse_base
	la		t2, arch_entry
	sv39_map_2M	bt_tbl_l0, bt_tbl_l1_1, jailhouse_base

	li	t0,	ATP_MODE_SV39
	j	setup_satp

sv48:
	beq		t4, t3, 1f
	sv48_map_2M	bt_tbl_l0, bt_tbl_l1_2, bt_tbl_l2_2
1:
	ld_constant	t2, jailhouse_base
	sv48_map_2M	bt_tbl_l0, bt_tbl_l1_0, bt_tbl_l2_0, jailhouse_base
	la		t2, arch_entry
	sv48_map_2M	bt_tbl_l0, bt_tbl_l1_1, bt_tbl_l2_1, jailhouse_base

	li	t0,	ATP_MODE_SV48
	j	setup_satp

sv57:
	beq		t4, t3, 1f
	sv57_map_2M	bt_tbl_l0, bt_tbl_l1_2, bt_tbl_l2_2, bt_tbl_l3_2
1:
	ld_constant	t2, jailhouse_base
	sv57_map_2M	bt_tbl_l0, bt_tbl_l1_0, bt_tbl_l2_0, bt_tbl_l3_0, \
			jailhouse_base
	la		t2, arch_entry
	sv57_map_2M	bt_tbl_l0, bt_tbl_l1_1, bt_tbl_l2_1, bt_tbl_l3_1, \
			jailhouse_base

	li	t0,	ATP_MODE_SV57
	j	setup_satp

setup_satp:
	ld_constant	a3, bt_tbl_l0
	add	a3, a2, a3
	srli	a3, a3, RISCV_PAGE_WIDTH
	slli	t0, t0, ATP_MODE_SHIFT
	or	a3, a3, t0

	sfence.vma	zero, zero
	csrrw	a4, satp, a3
	csrw	CSR_VSATP, a4

	ld_constant	t1, virtual_arch_entry
	/* leave pos independant code with this jump */
	jalr	t0, t1, 0

	# a0 ... cpuid
	# a2 ... physical - virtual address offset
	# a3 ... bootstrap satp
	# a4 ... linux page table root	(currently unused)
	# ra ... linux return address
	# t0 ... return address for call from arch_entry
virtual_arch_entry:
	# set up stack pointer -- this is hart-dependent

	# percpu data = pool + cpuid * percpu_size
	la	t1, __page_pool

	la	t6, hypervisor_header
	lw	t2, HEADER_PERCPU_SIZE (t6)

	mul	t2, t2, a0
	add	a1, t2, t1
	# a1 ... percpu data

	mv	t6, sp	# need to restore this later

	li	t1, PCPU_GUEST_REGS
	add	sp, a1, t1

	sd	t6, REG_SP (sp)
	sd	ra, REG_RA (sp)
	sd	t0, REG_T0 (sp)
	sd	gp, REG_GP (sp)
	sd	tp, REG_TP (sp)
	sd	s0, REG_S0 (sp)
	sd	s1, REG_S1 (sp)
	sd	s2, REG_S2 (sp)
	sd	s3, REG_S3 (sp)
	sd	s4, REG_S4 (sp)
	sd	s5, REG_S5 (sp)
	sd	s6, REG_S6 (sp)
	sd	s7, REG_S7 (sp)
	sd	s8, REG_S8 (sp)
	sd	s9, REG_S9 (sp)
	sd	s10, REG_S10 (sp)
	sd	s11, REG_S11 (sp)
	# fp is s0 -> fp is restored

	# vsstatus receives value of sstatus
	csr_from_csr	CSR_VSSTATUS, sstatus, t1

	li	t1, SSTATUS_INITIAL
	csrw	sstatus, t1

	load_csr	CSR_HIDELEG, t1, INITIAL_HIDELEG

	csr_from_csr	CSR_VSIE, sie, t1

	load_csr	sie, t1, INITIAL_SIE


	csr_from_csr	CSR_VSCAUSE, scause, t1
	csr_from_csr	CSR_VSTVAL, stval, t1

	csr_from_csr	CSR_VSSCRATCH, sscratch, t1
	csrw	sscratch, sp

	csr_from_csr	CSR_VSTVEC, stvec, t1
	la	t1, nested_exception_handler
	csrw	stvec, t1

	jal	ra, entry
	/* return from entry means an error occurred */

	mv	a1, a0
	mv	a0, sp
	j	riscv_deactivate_vmm
