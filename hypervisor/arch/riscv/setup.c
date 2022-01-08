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

int arch_init_early(void)
{
	int err;

	err = riscv_paging_cell_init(&root_cell);
	if (err)
		return err;

	parking_pt.root_paging = root_cell.arch.mm.root_paging;

	err = paging_create(&parking_pt, paging_hvirt2phys(riscv_park_loop),
			PAGE_SIZE, 0, PAGE_DEFAULT_FLAGS | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(U),
			PAGING_COHERENT | PAGING_NO_HUGE);

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
	csr_write(CSR_HCOUNTEREN, SCOUNTEREN_TM);
	csr_write(CSR_HTIMEDELTA, 0);

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

void __attribute__((noreturn)) riscv_deactivate_vmm(union registers *regs, int errcode)
{
	unsigned long linux_tables_offset, bootstrap_table_phys;
	register union registers *_regs;
	register int _errcode;
	u8 atp_mode;

	linux_tables_offset = symbol_offset((void*)hypervisor_header.initial_load_address);

	/* Do not return to VS-mode, rather return to S-Mode */
	csr_clear(CSR_HSTATUS, HSTATUS_SPV);

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
	_regs = (void*)regs + linux_tables_offset;
	_errcode = errcode;

	/* Before switching back, we need to jump to original load location */
	asm volatile(
			"add sp, sp, %0\n" /* Get stack under control */
			"jalr zero, 0(%1)\n" /* Jump to pre-virtual paging location */
			:
			: "r"(linux_tables_offset),
			  "r"(&&linux_tables + linux_tables_offset)
			:);
linux_tables:
	/*
	 * From now on, we can safely access stack variables, but we must not
	 * use any absolute addresses
	 */
	csr_from_csr(satp, CSR_VSATP);
	asm volatile("sfence.vma");
	/* From here on, Linux's paging is active. */

	/* Restore S-mode CSRs from VS-mode */
	csr_from_csr(stval, CSR_VSTVAL);
	csr_from_csr(scause, CSR_VSCAUSE);
	csr_from_csr(sscratch, CSR_VSSCRATCH);
	csr_from_csr(sie, CSR_VSIE);
	csr_from_csr(stvec, CSR_VSTVEC);
	csr_from_csr(sstatus, CSR_VSSTATUS);

	/*
	 * We came from an ecall, so we may clobber a0-a7. That's just fine, we
	 * can use them as scratch.
	 */
	asm volatile(
			/* scratch setup */
			"mv a2, %0\n" /* holds registers */
			"csrr a1, sepc\n" /* a1 hold return address */
			"addi a1, a1, 4\n" /* we came from an ecall */

			/* restore original sepc */
			//"csrr a0, csr_vsepc\n"
			"csrr a0, %2\n"
			"csrw sepc, a0\n"

			/* set return code */
			"mv a0, %1\n"

			/* restore registers */
			"ld ra, 8(a2)\n"
			"ld sp, 16(a2)\n"
			"ld gp, 24(a2)\n"
			"ld tp, 32(a2)\n"
			"ld t0, 40(a2)\n"
			"ld t1, 48(a2)\n"
			"ld t2, 56(a2)\n"
			"ld s0, 64(a2)\n"
			"ld s1, 72(a2)\n"
			/* Skip clobbers a0 - a7 */
			"ld s2, 144(a2)\n"
			"ld s3, 152(a2)\n"
			"ld s4, 160(a2)\n"
			"ld s5, 168(a2)\n"
			"ld s6, 176(a2)\n"
			"ld s7, 184(a2)\n"
			"ld s8, 192(a2)\n"
			"ld s9, 200(a2)\n"
			"ld s10, 208(a2)\n"
			"ld s11, 216(a2)\n"
			"ld t3, 224(a2)\n"
			"ld t4, 232(a2)\n"
			"ld t5, 240(a2)\n"
			"ld t6, 248(a2)\n"
			/* And we're done. */
			"jalr zero, a1, 0\n"
			:: "r"(_regs), "r"(_errcode), "i"(CSR_VSEPC) :);

	__builtin_unreachable();
}

void arch_cpu_restore(unsigned int cpu_id, int return_code)
{
}
