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

#include <jailhouse/paging.h>
#include <jailhouse/cell.h>
#include <jailhouse/string.h>
#include <jailhouse/unit.h>
#include <jailhouse/control.h>
#include <jailhouse/processor.h>
#include <jailhouse/printk.h>
#include <asm/irqchip.h>

#define PLIC_PRIO_BASE		0x0
#define PLIC_PENDING_BASE	0x1000
#define PLIC_ENABLE_BASE	0x2000
#define PLIC_ENABLE_OFF		0x80
#define PLIC_ENABLE_END		0x1f2000
#define PLIC_CTX_BASE		0x200000
#define PLIC_CTX_PRIO_TH	0x0
#define PLIC_CTX_CLAIM		0x4
#define PLIC_CTX_SZ		0x1000
#define PLIC_CTX_END		0x4000000

static inline s16 hart_to_context(unsigned int hartid)
{
	if (hartid > ARRAY_SIZE(SYSCONFIG_IRQCHIP.hart_to_context))
		return -1;

	return SYSCONFIG_IRQCHIP.hart_to_context[hartid];
}

static inline u16 plic_max_priority(void)
{
	return SYSCONFIG_IRQCHIP.max_priority;
}

static inline unsigned int plic_size(void)
{
	return SYSCONFIG_IRQCHIP.size;
}

static inline u32 plic_read(u32 reg)
{
	return mmio_read32(irqchip.base + reg);
}

static inline void plic_write(u32 reg, u32 value)
{
	mmio_write32(irqchip.base + reg, value);
}

static inline u32 plic_read_context(u32 context, u32 off)
{
	return plic_read(PLIC_CTX_BASE + context * PLIC_CTX_SZ + off);
}

static inline u16 plic_read_claim(u32 context)
{
	return plic_read_context(context, PLIC_CTX_CLAIM);
}

static inline u32 plic_en_reg(s16 context, unsigned int irq)
{
	u32 reg;

	reg = PLIC_ENABLE_BASE + (context * PLIC_ENABLE_OFF) +
	      (irq / IRQCHIP_BITS_PER_REG) * IRQCHIP_REG_SZ;

	return reg;
}

static inline u32 plic_read_enabled(s16 context, unsigned int irq)
{
	return plic_read(plic_en_reg(context, irq));
}

static inline void plic_write_enabled(s16 context, unsigned int irq, u32 val)
{
	plic_write(plic_en_reg(context, irq), val);
}

static inline bool plic_irq_is_enabled(s16 context, unsigned int irq)
{
	u32 en = plic_read_enabled(context, irq);

	return !!(en & IRQ_MASK(irq));
}

static inline void plic_enable_irq(s16 context, unsigned int irq)
{
	u32 val;

	val = plic_read_enabled(context, irq) | IRQ_MASK(irq);
	plic_write_enabled(context, irq, val);
}

static inline void plic_disable_irq_ctx(s16 context, unsigned int irq)
{
	u32 val;

	val = plic_read_enabled(context, irq) & ~IRQ_MASK(irq);
	plic_write_enabled(context, irq, val);
}

static int plic_claim_irq(void)
{
	unsigned long hart;
	unsigned short irq;
	int my_context;

	hart = phys_processor_id();

	/* Assume that phys_processor_id always returns something < 64 */
	my_context = hart_to_context(hart);
	if (my_context < 0)
		return -ENOSYS;

	irq = plic_read_claim(my_context);
	if (irq == 0) /* spurious IRQ, should not happen */
		return -EINVAL;

	if (irq > irqchip_max_irq())
		return -EINVAL;

	irqchip.pending[hart] = irq;

	return 0;
}

static inline void plic_passthru(const struct mmio_access *access)
{
	plic_write(access->address, access->value);
}

/* We must arrive here with the virq lock held */
static bool plic_inject_pending_virqs(void)
{
	struct public_per_cpu *pcpu = this_cpu_public();
	u32 idx, irq = 0;

	for (idx = 0; idx < ARRAY_SIZE(pcpu->virq.pending_bitmap); idx++) {
		irq = pcpu->virq.pending_bitmap[idx];
		if (!irq)
			continue;

		/*
		 * FIXME: For the moment, simply inject the first pending IRQ.
		 * Later, we need to prioritise those IRQs. Haha. Per call of
		 * this routine, we can only inject ONE single IRQ. That's not
		 * an issue, as the guest will trap again after acknowledging
		 * the last irq. So there will be no misses of pending IRQs.
		 */

		irq = ffsl(irq) + idx * 32;

		irqchip.pending[pcpu->phys_id] = irq;

		irq_bitmap_clear(pcpu->virq.pending_bitmap, irq);
		return true;
	}

	return false;
}

static inline enum mmio_result
plic_handle_context_claim(struct mmio_access *access, unsigned long hart)
{
	/* clear pending bit */
	if (!access->is_write) {
		access->value = irqchip.pending[hart];
		return MMIO_HANDLED;
	}

	/* claim write case */
	if (access->value != irqchip.pending[hart]) {
		printk("FATAL: Guest acknowledged non-pending IRQ %lu\n",
		       access->value);
		return MMIO_ERROR;
	}

	spin_lock(&this_cell()->arch.virq_lock);
	if (!irq_bitmap_test(this_cell()->arch.virq_present_bitmap, access->value))
		plic_write(access->address, access->value);

	/* Check if there's another physical IRQ pending */
	/* TODO: This is where we would need to prioritise vIRQs */
	irqchip.pending[hart] = plic_read(access->address);
	if (irqchip.pending[hart])
		goto out;

	/* TODO: vIRQ has the lowest prio at the moment */
	plic_inject_pending_virqs();
	if (irqchip.pending[hart])
		goto out;

	guest_clear_ext();
	ext_enable();

out:
	spin_unlock(&this_cell()->arch.virq_lock);
	return MMIO_HANDLED;
}

static enum mmio_result plic_handle_context(struct mmio_access *access)
{
	unsigned int cpu;
	unsigned long hart;
	int ctx;
	u64 addr;

	addr = access->address - PLIC_CTX_BASE;
	ctx = addr / PLIC_CTX_SZ;

	/*
	 * It is clear that a hart is allowed to access its own context.
	 * But we also need to allow accesses to context to neighboured
	 * harts within the cell.
	 *
	 * In (probably) 99% of all cases, the current active CPU will access
	 * its own context. So do this simple check first, and check other
	 * contexts of the cell (for loop) later. This results in a bit more
	 * complex code, but results in better performance.
	 */
	hart = phys_processor_id();
	if (hart_to_context(hart) == ctx)
		goto allowed;

	for_each_cpu_except(cpu, &this_cell()->cpu_set, this_cpu_id())
		if (hart_to_context(public_per_cpu(cpu)->phys_id) == ctx)
			goto allowed;

	return trace_error(MMIO_ERROR);

allowed:
	addr -= ctx * PLIC_CTX_SZ;
	if (addr == PLIC_CTX_CLAIM) {
		return plic_handle_context_claim(access, hart);
	} else if (addr == PLIC_CTX_PRIO_TH) {
		/* We land here if we permit the access */
		if (access->is_write)
			plic_passthru(access);
		else
			access->value = plic_read(access->address);
	} else {
		return MMIO_ERROR;
	}

	return MMIO_HANDLED;
}

static enum mmio_result plic_handle_prio(struct mmio_access *access)
{
	unsigned int irq;
	unsigned int prio = access->value;

	irq = access->address / IRQCHIP_REG_SZ;

	if (irqchip_virq_in_cell(this_cell(), irq)) {
		// TODO: Allow priorities
		printk("HART %u: PLIC: virq priorities not supported!\n", this_cpu_id());
		return MMIO_HANDLED;
	}

	/*
	 * When booting non-root Linux, it will set priorities of all IRQs.
	 * Hence, simply ignore non-allowed writes instead of crashing the
	 * cell.
	 */
	if (!irqchip_irq_in_cell(this_cell(), irq))
		return MMIO_HANDLED;

	/*
	 * Maybe we can abandon this check. The cell should know the max
	 * allowed value, so simply allow any value?
	 */
	if (prio > plic_max_priority())
		return MMIO_ERROR;

	plic_passthru(access);
	return MMIO_HANDLED;
}

static enum mmio_result plic_handle_enable(struct mmio_access *access)
{
	u32 *virq_enabled, irq_allowed_bitmap, virq_allowed_bitmap;
	struct public_per_cpu *pc;
	unsigned int idx, cpu;
	enum mmio_result res;
	short int ctx;

	ctx = (access->address - PLIC_ENABLE_BASE) / PLIC_ENABLE_OFF;

	/* Does the context even belong to one of the cell's CPUs? */
	for_each_cpu(cpu, &this_cell()->cpu_set) {
		pc = public_per_cpu(cpu);
		if (hart_to_context(pc->phys_id) == ctx)
			goto allowed;
	}

	return MMIO_ERROR;

allowed:
	/*
	 * Now we have to check if we have a read or write access. In case of
	 * reads, simply return the real value of the PLIC.
	 *
	 * In case of writes, compare against the irq_bitmap, if we're allowed
	 * to perform the write.
	 */
	idx = ((access->address - PLIC_ENABLE_BASE) % PLIC_ENABLE_OFF)
		* 8 / IRQCHIP_BITS_PER_REG;

	spin_lock(&this_cell()->arch.virq_lock);
	virq_enabled = &pc->virq.enabled_bitmap[idx];

	if (!access->is_write) {
		access->value = plic_read(access->address) | *virq_enabled;
		res = MMIO_HANDLED;
		goto out;
	}

	/* write case */
	irq_allowed_bitmap = this_cell()->arch.irq_bitmap[idx];
	virq_allowed_bitmap = this_cell()->arch.virq_present_bitmap[idx];

	if (access->value & ~(irq_allowed_bitmap | virq_allowed_bitmap)) {
		printk("FATAL: Cell enabled non-assigned IRQ\n");
		res = MMIO_ERROR;
		goto out;
	}

	*virq_enabled = access->value & virq_allowed_bitmap;

	/* Only forward physical IRQs to the PLIC */
	access->value &= irq_allowed_bitmap;
	plic_passthru(access);
	res = MMIO_HANDLED;

out:
	spin_unlock(&this_cell()->arch.virq_lock);
	return res;
}

static enum mmio_result plic_handler(void *arg, struct mmio_access *access)
{
	switch (access->address) {
	case REG_RANGE(PLIC_PRIO_BASE, PLIC_PENDING_BASE):
		return plic_handle_prio(access);
		break;

	case REG_RANGE(PLIC_ENABLE_BASE, PLIC_ENABLE_END):
		return plic_handle_enable(access);
		break;

	case REG_RANGE(PLIC_CTX_BASE, PLIC_CTX_END):
		if (access->address < plic_size())
			return plic_handle_context(access);
		break;

	default:
		break;
	}

	return MMIO_ERROR;
}

static int plic_init(void)
{
	unsigned short irq;
	unsigned int cpu;
	s16 context;

	/*
	 * If we check during early initialisation if all enabled IRQs belong
	 * to the root cell, then we don't need to check if an IRQ belongs to a
	 * cell on arrival.
	 */
	for_each_cpu(cpu, &root_cell.cpu_set) {
		context = hart_to_context(cpu);
		for (irq = 0; irq < irqchip_max_irq(); irq++) {
			if (plic_irq_is_enabled(context, irq) &&
			    !irqchip_irq_in_cell(&root_cell, irq)) {
				printk("Error: IRQ %u active in root cell\n",
				       irq);
				return trace_error(-EINVAL);
			}
		}
	}

	return 0;
}

static void plic_disable_irq(unsigned long hart, unsigned int irq)
{
	s16 ctx = hart_to_context(hart);
	if (ctx == -1)
		return;

	plic_disable_irq_ctx(ctx, irq);
}

static void plic_adjust_irq_target(struct cell *cell, unsigned int irq)
{
	unsigned long hart;
	unsigned int cpu;

	/* Disable the IRQ on each hart that does not belong to cell */
	for (hart = 0; hart < MAX_CPUS; hart++) {
		for_each_cpu(cpu, &cell->cpu_set)
			if (public_per_cpu(cpu)->phys_id == hart)
				goto cont;

		plic_disable_irq(hart, irq);
cont:
	}
}

static void plic_send_virq(struct cell *cell, unsigned int irq)
{
	struct public_per_cpu *pcpu;
	unsigned int cpu;
	bool send_event;

	spin_lock(&cell->arch.virq_lock);
	if (!irq_bitmap_test(cell->arch.virq_present_bitmap, irq)) {
		printk("vIRQ not present in destination\n");
		goto out;
	}

	send_event = false;
	for_each_cpu(cpu, &cell->cpu_set) {
		pcpu = public_per_cpu(cpu);
		if (irq_bitmap_test(pcpu->virq.enabled_bitmap, irq)) {
			irq_bitmap_set(pcpu->virq.pending_bitmap, irq);
			memory_barrier();
			send_event = true;
			break;
		}
	}

out:
	spin_unlock(&cell->arch.virq_lock);

	if (send_event)
		arch_send_event(pcpu);
}

static void plic_register_virq(struct cell *cell, unsigned int irq)
{
	irq_bitmap_set(cell->arch.virq_present_bitmap, irq);
}

static void plic_unregister_virq(struct cell *cell, unsigned int irq)
{
	unsigned int cpu;
	struct public_per_cpu *pcpu;

	spin_lock(&cell->arch.virq_lock);
	if (!irq_bitmap_test(cell->arch.virq_present_bitmap, irq))
		goto out;

	irq_bitmap_clear(cell->arch.virq_present_bitmap, irq);
	for_each_cpu(cpu, &cell->cpu_set) {
		pcpu = public_per_cpu(cpu);
		irq_bitmap_clear(pcpu->virq.enabled_bitmap, irq);
		irq_bitmap_clear(pcpu->virq.pending_bitmap, irq);

		if (irqchip.pending[pcpu->phys_id] == irq)
			irqchip.pending[pcpu->phys_id] = 0;
	}

out:
	spin_unlock(&cell->arch.virq_lock);
}

const struct irqchip irqchip_plic = {
	.init = plic_init,
	.claim_irq = plic_claim_irq,
	.adjust_irq_target = plic_adjust_irq_target,
	.mmio_handler = plic_handler,

	.send_virq = plic_send_virq,
	.register_virq = plic_register_virq,
	.unregister_virq = plic_unregister_virq,
	.inject_pending_virqs = plic_inject_pending_virqs,
};
