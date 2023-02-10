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

#include <jailhouse/control.h>
#include <jailhouse/entry.h>
#include <jailhouse/paging.h>
#include <jailhouse/percpu.h>
#include <asm/irqchip.h>
#include <asm/setup.h>

extern unsigned long bt_tbl_l0[PAGE_SIZE / sizeof(unsigned long)];

void riscv_park_loop(void);
void __attribute((noreturn))
__riscv_deactivate_vmm(union registers *regs, int errcode, bool from_ecall);

bool has_sstc;

static bool imsic_migration_done;

int arch_init_early(void)
{
	int err;

	err = riscv_paging_cell_init(&root_cell);
	if (err)
		return err;

	/* Always take VS-file 1, if IMSIC is available */
	root_cell.arch.vs_file = imsic_base() ? 1 : 0;

	parking_pt.root_paging = root_cell.arch.mm.root_paging;

	err = paging_create(&parking_pt, paging_hvirt2phys(riscv_park_loop),
			PAGE_SIZE, 0, PAGE_DEFAULT_FLAGS | RISCV_PTE_FLAG(G) |
			RISCV_PTE_FLAG(U), PAGING_COHERENT | PAGING_NO_HUGE);

	return 0;
}

static void imsic_migrate_to_vs(unsigned char reg, bool set)
{
	u64 val;

	csr_write(CSR_SISELECT, reg);
	csr_write(CSR_VSISELECT, reg);

	val = csr_read(CSR_SIREG);
	csr_write(CSR_SIREG, 0);
	if (set)
		csr_set(CSR_VSIREG, val);
	else
		csr_write(CSR_VSIREG, val);
}

static void imsic_migrate_to_s(unsigned char reg, bool set)
{
	u64 val;

	csr_write(CSR_VSISELECT, reg);
	csr_write(CSR_SISELECT, reg);

	val = csr_read(CSR_VSIREG);
	csr_write(CSR_VSIREG, 0);

	if (set)
		csr_set(CSR_SIREG, val);
	else
		csr_write(CSR_SIREG, val);
}

static void imsic_migrate_regs(void (*migrator)(unsigned char, bool))
{
	unsigned short eiep;

	migrator(CSR_SIREG_EIDELIVERY, false);
	migrator(CSR_SIREG_EITHRESHOLD, false);
	for (eiep = 0; eiep < (irqchip_max_irq() + 63) / 64; eiep++) {
		migrator(CSR_SIREG_EIE0 + 2 * eiep, false);
		migrator(CSR_SIREG_EIP0 + 2 * eiep, true);
	}
}

int arch_cpu_init(struct per_cpu *cpu_data)
{
	struct public_per_cpu *ppc = &cpu_data->public;
	unsigned long final_pt;

	spin_init(&ppc->control_lock);

	ppc->reset = false;
	ppc->park = false;
	ppc->wait_for_power_on = false;

	ppc->phys_id =
		jailhouse_cell_cpus(root_cell.config)[cpu_data->public.cpu_id]
		.phys_id;
	ppc->hsm.state = STARTED;

	final_pt = paging_hvirt2phys(&ppc->root_table_page);
	enable_mmu_satp(hv_atp_mode, final_pt);

	cpu_set_vs_file(root_cell.arch.vs_file);

	/*
	 * If VGEIN is set, then we have an IMSIC. Migrate its registers to the
	 * new VS-mode file. We need to do this very early, in case we have an
	 * IMSIC, as this migration works on per-cpu CSR registers. Hence, we
	 * can't do it in aplic_init().
	 */
	if (root_cell.arch.vs_file)
		ppc->imsic_base =
			imsic_base() + ppc->phys_id * imsic_stride_size();

	return 0;
}

void __attribute__ ((noreturn)) arch_cpu_activate_vmm(void)
{
	union registers *regs;
	unsigned long tmp;

	regs = &this_cpu_data()->guest_regs;

	/* VSBE = 0 -> VS-Mode mem accesses are LE */
	csr_set(CSR_HSTATUS,
		HSTATUS_SPV | /* Return to VS-Mode */
		(2ULL << HSTATUS_VSXL_SHIFT)); /* xlen = 64 */

	csr_write(CSR_HEDELEG,
		(1UL << EXC_INST_MISALIGNED) |
		(1UL << EXC_INST_ACCESS) |
		(1UL << EXC_INST_ILLEGAL) |
		(1UL << EXC_BREAKPOINT) |
		(1UL << EXC_LOAD_ACCESS_MISALIGNED) |
		(1UL << EXC_LOAD_ACCESS) |
		(1UL << EXC_AMO_ADDRESS_MISALIGNED) |
		(1UL << EXC_STORE_ACCESS) |
		(1UL << EXC_SYSCALL) |
		(1UL << EXC_INST_PAGE_FAULT) |
		(1UL << EXC_LOAD_PAGE_FAULT) |
		(1UL << EXC_STORE_PAGE_FAULT));

	csr_write(CSR_HGEIE, 0);
	csr_write(CSR_HCOUNTEREN, SCOUNTEREN_CY | SCOUNTEREN_TM);
	csr_write(CSR_HTIMEDELTA, 0);

	tmp = csr_read(sip);
	csr_write(CSR_HVIP, tmp << VSIP_TO_HVIP_SHIFT); /* reinject pending */

	/* try to enable SSTC extension, if available */
	csr_write(CSR_HENVCFG, ENVCFG_STCE);
	/* STCE is WARL, check its presence */
	has_sstc = !!(csr_read(CSR_HENVCFG) & ENVCFG_STCE);
	/*
	 * If we discovered SSTC, then disable the S-Mode Timer and migrate it
	 * to VS-Mode. Even if the guest doesn't use SSTC this is okay, as the
	 * Timer will arrive regularly.
	 */
	if (has_sstc) {
		tmp = csr_read(CSR_STIMECMP);
		csr_write(CSR_VSTIMECMP, tmp);
		csr_write(CSR_STIMECMP, -1);
		timer_disable();
	}

	riscv_paging_vcpu_init(&this_cell()->arch.mm);

	/* Return value */
	regs->a0 = 0;

	csr_write(sepc, regs->ra); /* We will use sret, so move ra->sepc */

	/*
	 * While activating jailhouse, a MSI(-X) might still arrive at the
	 * S-file until the MSI's address is relocated. In this case, S-Mode
	 * will still hold pending interrupts. Here, all IRQs (legacy IRQs, as
	 * well as MSIs) are migrated, and we can safely migrate all pending
	 * IRQs from the old S-Mode file to the VS-File.
	 */
	 if (csr_read(CSR_HSTATUS) & HSTATUS_VGEIN) {
		imsic_migrate_regs(imsic_migrate_to_vs);
		imsic_migration_done = true;

		/*
		 * In case we have an IMSIC, there must be no external IRQs any
		 * longer, so disable them for the S-Mode Hypervisor
		 */
		ext_disable();
	}

	tmp = csr_swap(sscratch, regs->sp);
	asm volatile("mv sp, %0\n"
		     "j vmreturn\n" : : "r"(tmp));

	__builtin_unreachable();
}

static unsigned long symbol_offset(const void *addr)
{
	return (unsigned long)addr - (unsigned long)&hypervisor_header;
}

void __attribute__((noreturn))
riscv_deactivate_vmm(union registers *regs, int errcode, bool from_ecall)
{
	void __attribute__((noreturn))
		(*deactivate_vmm)(union registers *, int, bool);
	unsigned long linux_tables_offset, bootstrap_table_phys;
	u64 timecmp;
	u8 atp_mode;

	linux_tables_offset =
		symbol_offset((void*)hypervisor_header.initial_load_address);

	/* Do not return to VS-mode, rather return to S-Mode */
	csr_clear(CSR_HSTATUS, HSTATUS_SPV);

	/* Migrate the timer back to S-Mode */
	if (has_sstc) {
		timecmp = csr_read(CSR_VSTIMECMP);
		csr_write(CSR_VSTIMECMP, -1);
		csr_write(CSR_STIMECMP, timecmp);
	}

	/* Migrate the VS-Mode IMSIC file back to the S-Mode file */
	if ((csr_read(CSR_HSTATUS) & HSTATUS_VGEIN) && imsic_migration_done)
		imsic_migrate_regs(imsic_migrate_to_s);

	/*
	 * We don't know which page table is currently active. So in any case,
	 * just jump back to the bootstrap tables, as they contain the old
	 * Linux mapping of Jailhouse
	 */
	bootstrap_table_phys = system_config->hypervisor_memory.phys_start +
			       symbol_offset(&bt_tbl_l0);
	/* Take Linux's MMU mode */
	atp_mode = csr_read(CSR_VSATP) >> ATP_MODE_SHIFT;
	enable_mmu_satp(atp_mode, bootstrap_table_phys);

	/*
	 * next access to regs will be under Linux's old page table, so amend
	 * the address
	 */
	regs = (void*)regs + linux_tables_offset;
	deactivate_vmm = __riscv_deactivate_vmm + linux_tables_offset;

	/* Before switching back, we need to jump to original load location */
	/* Get stack under control */
	asm volatile("add sp, sp, %0\n" :: "r"(linux_tables_offset) :);
	deactivate_vmm(regs, errcode, from_ecall);
}

void arch_cpu_restore(unsigned int cpu_id, int return_code)
{
}
