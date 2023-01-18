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

#include <asm/processor.h>
#include <asm/spinlock.h>

#define STACK_SIZE	(PAGE_SIZE << 2)

enum sbi_hart_state {
	STARTED		= 0,
	STOPPED		= 1,
	START_PENDING	= 2,
	STOP_PENDING	= 3,
	SUSPENDED	= 4,
	SUSPEND_PENDING = 5,
	RESUME_PENDING	= 6,
};

#define ARCH_PUBLIC_PERCPU_FIELDS					\
	unsigned long phys_id;						\
	enum {								\
		IPI_CAUSE_NONE,						\
		IPI_CAUSE_GUEST,					\
		IPI_CAUSE_MGMT,						\
	} ipi_cause;							\
	spinlock_t control_lock;					\
	struct {							\
		enum sbi_hart_state state;				\
		unsigned long start_addr;				\
		unsigned long opaque;					\
	} hsm;								\
	bool wait_for_power_on;						\
	bool reset;							\
	bool park;							\
	struct {							\
		u32 enabled_bitmap[MAX_IRQS / (sizeof(u32) * 8)];	\
		u32 pending_bitmap[MAX_IRQS / (sizeof(u32) * 8)];	\
		unsigned int aplic_pending;				\
	} virq;

#define ARCH_PERCPU_FIELDS
