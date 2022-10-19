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
#include <asm/csr64.h>
#include <asm/setup.h>

extern unsigned long bt_tbl_l0[PAGE_SIZE / sizeof(unsigned long)];

void riscv_park_loop(void);
void __attribute((noreturn))
__riscv_deactivate_vmm(union registers *regs, int errcode, bool from_ecall);

static bool has_stce;

int arch_init_early(void)
{
	int err;

	err = riscv_paging_cell_init(&root_cell);
	if (err)
		return err;

	parking_pt.root_paging = root_cell.arch.mm.root_paging;

	err = paging_create(&parking_pt, paging_hvirt2phys(riscv_park_loop),
			PAGE_SIZE, 0, PAGE_DEFAULT_FLAGS | RISCV_PTE_FLAG(G) |
			RISCV_PTE_FLAG(U), PAGING_COHERENT | PAGING_NO_HUGE);

	return 0;
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

	return 0;
}

void __attribute__ ((noreturn)) arch_cpu_activate_vmm(void)
{
	union registers *regs;
	unsigned long tmp;

	regs = &this_cpu_data()->guest_regs;

	/* VSBE = 0 -> VS-Mode mem accesses are LE */
	csr_write(CSR_HSTATUS,
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

	/* try to enable SSTC extension, if available */
	csr_write(CSR_HENVCFG, ENVCFG_STCE);
	/* STCE is WARL, check its presence */
	has_stce = !!(csr_read(CSR_HENVCFG) & ENVCFG_STCE);
	/*
	 * If we discovered STCE, then disable the S-Mode Timer and migrate it
	 * to VS-Mode. Even if the guest doesn't use STCE this is okay, as the
	 * Timer will arrive regularly.
	 */
	if (has_stce) {
		tmp = csr_read(CSR_STIMECMP);
		csr_write(CSR_VSTIMECMP, tmp);
		csr_write(CSR_STIMECMP, -1);
	}

	tmp = csr_read(sip);
	csr_write(sip, tmp); /* clear pending */
	csr_write(CSR_HVIP, tmp); /* reinject pending */

	riscv_paging_vcpu_init(&this_cell()->arch.mm);

	/* Return value */
	regs->a0 = 0;

	csr_write(sepc, regs->ra); /* We will use sret, so move ra->sepc */

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
		(*deactivate_vmm)(union registers *regs, int errcode, bool from_ecall);
	unsigned long linux_tables_offset, bootstrap_table_phys;
	u64 timecmp;
	u8 atp_mode;


	linux_tables_offset = symbol_offset((void*)hypervisor_header.initial_load_address);

	/* Do not return to VS-mode, rather return to S-Mode */
	csr_clear(CSR_HSTATUS, HSTATUS_SPV);

	/* Migrate the timer back to S-Mode */
	if (has_stce) {
		timecmp = csr_read(CSR_VSTIMECMP);
		csr_write(CSR_VSTIMECMP, -1);
		csr_write(CSR_STIMECMP, timecmp);
	}

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

	/* next access to regs will be under Linux's old page table, so amend the address */
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
