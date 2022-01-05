/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* Shared definitions for (A)PLIC */
#define IRQCHIP_REG_SZ		4
#define IRQCHIP_BITS_PER_REG	(IRQCHIP_REG_SZ * 8)

#define IRQ_BIT(irq)		((irq) % IRQCHIP_BITS_PER_REG)
#define IRQ_MASK(irq)		(1 << IRQ_BIT(irq))

#ifndef __ASSEMBLY__

#define REG_RANGE(A, B)		(A)...((B) - IRQCHIP_REG_SZ)

#define SYSCONFIG_IRQCHIP	system_config->platform_info.riscv.irqchip

struct irqchip {
	int (*init)(void);
	enum mmio_result (*mmio_handler)(void *arg, struct mmio_access *access);
	int (*claim_irq)(void);
	void (*adjust_irq_target)(struct cell *cell, unsigned int irq);

	void *base;
	unsigned long pending[MAX_CPUS];
};

extern struct irqchip irqchip;

static inline unsigned long imsic_base(void)
{
	return SYSCONFIG_IRQCHIP.imsic_base;
}

static inline unsigned long imsic_size(void)
{
	return SYSCONFIG_IRQCHIP.imsic_size;
}

static inline unsigned long imsic_stride_size(void)
{
	return SYSCONFIG_IRQCHIP.imsic_stride;
}

static inline unsigned long irqchip_type(void)
{
	return SYSCONFIG_IRQCHIP.type;
}

static inline unsigned long irqchip_phys(void)
{
	return SYSCONFIG_IRQCHIP.base_address;
}

static inline unsigned long irqchip_size(void)
{
	return SYSCONFIG_IRQCHIP.size;
}

static inline short irqchip_max_irq(void)
{
	return SYSCONFIG_IRQCHIP.max_irq;
}

static inline void irq_bitmap_set(u32 *bitmap, unsigned int irq)
{
	bitmap[irq / IRQCHIP_BITS_PER_REG] |= IRQ_MASK(irq);
}

static inline void irq_bitmap_clear(u32 *bitmap, unsigned int irq)
{
	bitmap[irq / IRQCHIP_BITS_PER_REG] &= ~IRQ_MASK(irq);
}

static inline bool irq_bitmap_test(u32 *bitmap, unsigned short irq)
{
	u32 val;

	if (irq >= irqchip_max_irq())
		return false;

	val = bitmap[irq / IRQCHIP_BITS_PER_REG];

	return !!(val & IRQ_MASK(irq));
}

static inline bool irqchip_irq_in_cell(struct cell *cell, unsigned int irq)
{
	return irq_bitmap_test(cell->arch.irq_bitmap, irq);
}

int irqchip_set_pending(void);

#endif /* __ASSEMBLY__ */
