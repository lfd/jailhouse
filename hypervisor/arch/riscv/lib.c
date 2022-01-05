/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/printk.h>
#include <jailhouse/processor.h>
#include <jailhouse/percpu.h>

unsigned long phys_processor_id(void)
{
	return this_cpu_public()->phys_id;
}

unsigned int cpu_by_phys_processor_id(u64 phys)
{
	const struct jailhouse_cpu *map = jailhouse_cell_cpus(root_cell.config);
	unsigned int num_cpus = root_cell.config->num_cpus;
	unsigned int cpu;

	for (cpu = 0; cpu < num_cpus; cpu++, map++)
		if (map->phys_id == phys)
			return cpu;

	/* this should never happen */
	panic_printk("FATAL: unknown HART ID %llu\n", phys);
	panic_stop();
}
