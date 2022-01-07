/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/printk.h>
#include <asm/sbi.h>

int arch_cell_create(struct cell *cell)
{
	return -ENOSYS;
}

int arch_map_memory_region(struct cell *cell,
			   const struct jailhouse_memory *mem)
{
	return -ENOSYS;
}

int arch_unmap_memory_region(struct cell *cell,
			     const struct jailhouse_memory *mem)
{
	return -ENOSYS;
}

void arch_check_events(void)
{
}

void arch_flush_cell_vcpu_caches(struct cell *cell)
{
}

void arch_cell_destroy(struct cell *cell)
{
}

void arch_cell_reset(struct cell *cell)
{
}

void arch_prepare_shutdown(void)
{
}

void __attribute__((noreturn)) arch_panic_stop(void)
{
	while (1);
}

void arch_panic_park(void)
{
}

void arch_reset_cpu(unsigned int const cpu_id)
{
}

void arch_park_cpu(unsigned int cpu_id)
{
}

void arch_send_event(struct public_per_cpu *target_data)
{
	struct sbiret result;

	target_data->ipi_cause = IPI_CAUSE_MGMT;
	memory_barrier();

	result = sbi_send_ipi(1UL << (target_data->phys_id % BITS_PER_LONG),
			      target_data->phys_id / BITS_PER_LONG);
	if (result.error != SBI_SUCCESS) {
		printk("IPI send to HART %lu failed: %ld\n",
		       target_data->phys_id, result.error);
		panic_stop();
	}
}
