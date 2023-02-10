/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/paging.h>
#include <jailhouse/printk.h>
#include <jailhouse/string.h>
#include <asm/control.h>
#include <asm/irqchip.h>
#include <asm/sbi.h>

extern void __attribute__((noreturn)) riscv_park_loop(void);

static void riscv_cpu_reset(unsigned long pc, unsigned long a0,
			    unsigned long a1)
{
	union registers *regs;

	regs = &this_cpu_data()->guest_regs;

	memset(regs, 0, sizeof(union registers));
	regs->pc = pc;
	regs->a0 = a0;
	regs->a1 = a1;

	csr_write(sepc, regs->pc);
	csr_write(CSR_VSATP, 0);
	csr_write(CSR_HIE, 0);
	csr_write(CSR_HIP, 0);
	csr_write(CSR_VSSTATUS, 0);

	csr_set(sstatus, SR_SPP); /* Return to VS-Mode */
}

void arch_check_events(void)
{
	struct public_per_cpu *pcpu;

	pcpu = this_cpu_public();

	spin_lock(&pcpu->control_lock);

	if (pcpu->suspend_cpu) {
		pcpu->cpu_suspended = true;

		spin_unlock(&pcpu->control_lock);

		while (pcpu->suspend_cpu)
			cpu_relax();

		spin_lock(&pcpu->control_lock);
	}

	pcpu->cpu_suspended = false;

	if (pcpu->wait_for_power_on) {
		pcpu->wait_for_power_on = false;
		goto out;
	}

	if (pcpu->hsm.state == START_PENDING) {
		pcpu->reset = false;
		riscv_cpu_reset(pcpu->hsm.start_addr, pcpu->phys_id,
				pcpu->hsm.opaque);
		riscv_paging_vcpu_init(&this_cell()->arch.mm);
		pcpu->hsm.state = STARTED;
	} else if (pcpu->park) {
		riscv_park_cpu();
	}

	if (pcpu->reset) {
		pcpu->reset = false;
		riscv_cpu_reset(0x0, pcpu->phys_id, 0);
		riscv_paging_vcpu_init(&this_cell()->arch.mm);
	}

out:
	pcpu->park = false;
	spin_unlock(&pcpu->control_lock);
}

int arch_map_memory_region(struct cell *cell,
			   const struct jailhouse_memory *mem)
{
	return paging_create(
		&cell->arch.mm,
		mem->flags & JAILHOUSE_MEM_COMM_REGION ?
		paging_hvirt2phys (&cell->comm_page) : mem->phys_start,
		mem->size, mem->virt_start,
		PAGE_GUEST_PRESENT_FLAGS |
		(mem->flags & JAILHOUSE_MEM_READ ? RISCV_PTE_FLAG(R) : 0) |
		(mem->flags & JAILHOUSE_MEM_WRITE ? RISCV_PTE_FLAG(W) : 0) |
		(mem->flags & JAILHOUSE_MEM_EXECUTE ? RISCV_PTE_FLAG(X) : 0),
		PAGING_COHERENT |
		(mem->flags & JAILHOUSE_MEM_NO_HUGEPAGES ? 0 : PAGING_HUGE));
}

int arch_unmap_memory_region(struct cell *const cell,
			     const struct jailhouse_memory *mem_original)
{
	return paging_destroy(&cell->arch.mm, mem_original->virt_start,
			      mem_original->size, PAGING_COHERENT);
}

void arch_flush_cell_vcpu_caches(struct cell *const cell)
{
	/*
	 * The necessary TLB invalidation has already been performed in the
	 * map/unmap routines. Doing it here would require the entire cell's
	 * TLB to be flushed, because this function does not receive
	 * information about the memory segment to invalidate. That would be
	 * overkill (although the current unmap_region implementation does this
	 * nonetheless, owing to other API shortcomings).
	 */
}

int arch_cell_create(struct cell *const cell)
{
	struct public_per_cpu *ppc;
	unsigned int cpu;

	cell->arch.mm.root_paging = riscv_Sv39x4;
	cell->arch.mm.root_table =
		page_alloc_aligned(&mem_pool, CELL_ROOT_PT_PAGES);

	/* Always take VS file 1 for every cell, if IMSIC is available */
	cell->arch.vs_file = imsic_base() ? 1 : 0;

	for_each_cpu(cpu, &cell->cpu_set) {
		ppc = public_per_cpu(cpu);
		ppc->wait_for_power_on = false;
		ppc->park = true;
		ppc->reset = false;
	}

	spin_init(&cell->arch.virq_lock);

	if (!cell->arch.mm.root_table)
		return -ENOMEM;

	return 0;
}

void arch_cell_destroy(struct cell *const cell)
{
	unsigned int cpu;
	struct public_per_cpu *ppc;

	page_free(&mem_pool, cell->arch.mm.root_table, CELL_ROOT_PT_PAGES);

	for_each_cpu(cpu, &cell->cpu_set) {
		ppc = public_per_cpu(cpu);
		ppc->wait_for_power_on = false;
		ppc->park = true;
		ppc->reset = false;
	}
}

void arch_cell_reset(struct cell *const cell)
{
        struct jailhouse_comm_region *comm_region;
	unsigned int first = first_cpu(&cell->cpu_set);
	struct public_per_cpu *pcpu;
	unsigned int cpu;

	comm_region = &cell->comm_page.comm_region;
	comm_region->timebase_frequency =
		system_config->platform_info.riscv.timebase_frequency;

	/*
	 * All CPUs except the first one shall not be started, they shall park
	 */
	pcpu = public_per_cpu(first);
	pcpu->wait_for_power_on = false;
	pcpu->park = false;
	for_each_cpu_except(cpu, &cell->cpu_set, first) {
		pcpu = public_per_cpu(cpu);
		pcpu->wait_for_power_on = true;
		pcpu->hsm.state = STOPPED;
	}
}

void arch_reset_cpu(unsigned int const cpu_id)
{
	public_per_cpu(cpu_id)->reset = true;

	resume_cpu(cpu_id);
}

void arch_park_cpu(unsigned int const cpu_id)
{
	struct public_per_cpu *pc = public_per_cpu(cpu_id);

	pc->park = true;

	resume_cpu(cpu_id);
}

void arch_prepare_shutdown(void)
{
}

void __attribute__((noreturn)) arch_panic_stop(void)
{
	/* No need to check return code here */
	sbi_hart_stop();

	/*
	 * If this happens, which should never be the case, then let the CPU
	 * execute the park loop.
	 */
	riscv_park_loop();
}

void arch_panic_park(void)
{
	ext_disable();
	riscv_park_cpu();
}

void arch_send_event(struct public_per_cpu *target_data)
{
	struct sbiret result;

	/*
	 * For sending an event to a remote HART, we need two conditions to be
	 * set:
	 *   1. We have to grab the CPU's control lock. Having the control lock
	 *      ensures that the CPU is currently not processing any other IPI.
	 *      If the CPU is processing an IPI, then we have to busy wait for
	 *      completion.
	 *   2. target_data->ipi_cause must be NONE at the moment of grabbing
	 *      the lock. This ensures that no other CPU enqueued an IPI
	 *      before.
	 */
retry:
	spin_lock(&target_data->control_lock);
	if (target_data->ipi_cause != IPI_CAUSE_NONE) {
		spin_unlock(&target_data->control_lock);
		goto retry;
	}

	target_data->ipi_cause = IPI_CAUSE_MGMT;
	memory_barrier();
	spin_unlock(&target_data->control_lock);

	result = sbi_send_ipi(1UL << (target_data->phys_id % BITS_PER_LONG),
			      target_data->phys_id / BITS_PER_LONG);
	if (result.error != SBI_SUCCESS) {
		printk("IPI send to HART %lu failed: %ld\n",
		       target_data->phys_id, result.error);
		panic_stop();
	}
}

void riscv_park_cpu(void)
{
	this_cpu_public()->hsm.state = STOPPED;

	cpu_set_vs_file(this_cell()->arch.vs_file);

	/*
	 * When parking a HART, we let the CPU spin in a wfi loop. To avoid
	 * that we busily spin that loop, deactivate interrupts while sitting
	 * on the parking page.
	 */
	sbi_set_timer(-1);

	riscv_paging_vcpu_init(&parking_pt);
	riscv_cpu_reset(0, 0, 0);
}
