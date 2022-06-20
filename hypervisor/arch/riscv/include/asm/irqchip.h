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
	void (*disable_irq)(unsigned long hart, unsigned int irq);
	unsigned long (*claim_irq)(void);

	void *base;
	unsigned long pending[MAX_CPUS];
};

extern struct irqchip irqchip;

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

static inline u16 irqchip_max_irq(void)
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

static inline bool irq_bitmap_test(u32 *bitmap, unsigned int irq)
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

static inline bool irqchip_virq_in_cell(struct cell *cell, unsigned int irq)
{
	return irq_bitmap_test(cell->arch.virq_present_bitmap, irq);
}

static inline void guest_inject_ext(void)
{
	csr_set(CSR_HVIP, (1 << IRQ_S_EXT) << VSIP_TO_HVIP_SHIFT);
}

static inline void ext_disable(void)
{
	csr_clear(sie, IE_EIE);
}

int irqchip_set_pending(void);

void irqchip_register_virq(unsigned int irq);
void irqchip_unregister_virq(unsigned int irq);
void irqchip_send_virq(struct cell *cell, unsigned int irq);
void irqchip_process_pending_virqs(void);
bool irqchip_inject_pending_virqs(void);

#endif /* __ASSEMBLY__ */
