/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define JAILHOUSE_SIGNATURE	"JAILHOUS"

/**
 * @ingroup Setup
 * @{
 */

/**
 * Hypervisor entry point.
 *
 * @see arch_entry
 */
typedef int (*jailhouse_entry)(unsigned int);

#define CONSOLE_SIZE 0x1000

struct jailhouse_console {
       unsigned short start;
       char content[CONSOLE_SIZE - sizeof(unsigned short)];
} __attribute__((aligned(CONSOLE_SIZE)));

/**
 * Hypervisor description.
 * Located at the beginning of the hypervisor binary image and loaded by
 * the driver (which also initializes some fields).
 */
struct jailhouse_header {
	/** Signature "JAILHOUS" used for basic validity check of the
	 * hypervisor image.
	 * @note Filled at build time. */
	char signature[8];
	/** Size of hypervisor core.
	 * It starts with the hypervisor's header and ends after its bss
	 * section. Rounded up to page boundary.
	 * @note Filled at build time. */
	unsigned long core_size;
	/** Size of the per-CPU data structure.
	 * @note Filled at build time. */
	unsigned long percpu_size;
	/** Entry point (arch_entry()).
	 * @note Filled at build time. */
	int (*entry)(unsigned int);

	/** Configured maximum logical CPU ID + 1.
	 * @note Filled by Linux loader driver before entry. */
	unsigned int max_cpus;
	/** Number of online CPUs that will call the entry function.
	 * @note Filled by Linux loader driver before entry. */
	unsigned int online_cpus;
	/** Virtual base address of the debug console device (if used).
	 * @note Filled by Linux loader driver before entry. */
	void *debug_console_base;
	/** Virtual address of the clock gate register (if used).
	 * @note Filled by Linux loader driver before entry. */
	void *debug_clock_reg;
};
