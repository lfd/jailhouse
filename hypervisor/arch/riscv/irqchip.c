/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/unit.h>
#include <jailhouse/printk.h>
#include <jailhouse/processor.h>
#include <asm/irqchip.h>

/* Could also be used for arm-common/irqchip.c */
#define IRQCHIP_PINS \
	(sizeof(((struct jailhouse_irqchip *)0)->pin_bitmap) * 8)

#define IRQ_BITMAP_PINS \
	(sizeof(((struct cell *)0)->arch.irq_bitmap) * 8)

extern const struct irqchip irqchip_plic;

struct irqchip irqchip;

static unsigned int irqchip_mmio_count_regions(struct cell *cell)
{
	return 1;
}

static void irqchip_config_commit(struct cell *cell)
{
	unsigned int irq, n;

	if (!cell)
		return;

	if (cell == &root_cell)
		return;

	for (irq = 0; irq < irqchip_max_irq(); irq++) {
		/*
		 * Three possibilities:
		 *   1. IRQ belongs to root cell and was removed (cell
		 *      creation)
		 *   2. IRQ belonged to non-root cell and was assigned back to
		 *      non-root cell (cell destruction)
		 *   3. IRQ belonged to non-root cell and is simply gone
		 *      (belongs to no one)
		 *
		 * IRQ-Bitmaps are already updated accordingly. All we have to
		 * do is to ensure that the IRQ is disabled. That's all.
		 */
		if (!irqchip_irq_in_cell(cell, irq))
			continue;

		/* Disable the IRQ for each HART. */
		// TODO: Can we use for_each_cpu here?
		for (n = 0; n < MAX_CPUS; n++) {
			irqchip.disable_irq(n, irq);
		}
	}
}

static int irqchip_cell_init(struct cell *cell)
{
	const struct jailhouse_irqchip *chip;
	unsigned int n, pos;

	mmio_region_register(cell, irqchip_phys(), irqchip_size(),
			     irqchip.mmio_handler, cell);

	/*
	 * TODO: Do we need that, or can we assume that this arrives already
	 * zeroed?
	 */
	memset(cell->arch.irq_bitmap, 0, sizeof(cell->arch.irq_bitmap));
	memset(cell->arch.virq_present_bitmap, 0,
	       sizeof(cell->arch.virq_present_bitmap));

	for_each_irqchip(chip, cell->config, n) {
		/* Only support one single PLIC at the moment */
		if (chip->address !=
		    system_config->platform_info.riscv.irqchip.base_address)
			return trace_error(-EINVAL);

		if (chip->pin_base % 32 != 0 ||
		    chip->pin_base + IRQCHIP_PINS > IRQ_BITMAP_PINS)
			return trace_error(-EINVAL);

		for (pos = 0; pos < ARRAY_SIZE(chip->pin_bitmap); pos++)
			cell->arch.irq_bitmap[chip->pin_base / 32 + pos] |=
				chip->pin_bitmap[pos];
	}

	/* This logic is shared with arm-common */
	if (cell == &root_cell)
		return 0;

	for_each_irqchip(chip, cell->config, n) {
		// TODO: Check if IRQs are disabled before removing them
		for (pos = 0; pos < ARRAY_SIZE(chip->pin_bitmap); pos++)
			root_cell.arch.irq_bitmap[chip->pin_base / 32 + pos] &=
				~chip->pin_bitmap[pos];
	}

	return 0;
}

static inline bool guest_ext_pending(void)
{
	return !!(csr_read(CSR_HVIP) &
			((1 << IRQ_S_EXT) << VSIP_TO_HVIP_SHIFT));
}

int irqchip_set_pending(void)
{
	unsigned long irq;

	this_cpu_public()->stats[JAILHOUSE_CPU_STAT_VMEXITS_VIRQ]++;

	irq = irqchip.claim_irq();
	if (irq == 0) /* spurious IRQ, should not happen */
		return -EINVAL;

	if (irq > irqchip_max_irq())
		return -EINVAL;

	irqchip.pending[phys_processor_id()] = irq;
	/*
	 * We can directly inject the IRQ into the guest if the IRQ is not
	 * pending, because we know that the IRQ is enabled, otherwise we
	 * wouldn't have received it
	 */
	guest_inject_ext();

	/*
	 * Don't claim complete! This must be done by the guest. We will handle
	 * that in plic_handler(). In the meanwhile we simply deactivate S-Mode
	 * External IRQs, and reenable them when the guest claims it. In this
	 * way, we only need to store one pending IRQ per hart.
	 */
	ext_disable();

	return 0;
}

static int irqchip_init(void)
{
	int err;

	switch (irqchip_type()) {
		case JAILHOUSE_RISCV_PLIC:
			irqchip = irqchip_plic;
			break;

		default:
			return trace_error(-ENOSYS);
	}

	irqchip.base = paging_map_device(irqchip_phys(), irqchip_size());
	if (!irqchip.base)
		return -ENOMEM;


	err = irqchip_cell_init(&root_cell);
	if (err)
		return err;

	err = irqchip.init();

	return err;
}

static void irqchip_shutdown(void)
{
	if (!irqchip.base)
		return;

	paging_unmap_device(irqchip_phys(), irqchip.base, irqchip_size());
}

static void irqchip_cell_exit(struct cell *cell)
{
	const struct jailhouse_irqchip *chip;
	unsigned int n, pos;

	mmio_region_unregister(cell, irqchip_phys());

	/* set all pins of the old cell in the root cell */
	for_each_irqchip(chip, cell->config, n)
		for (pos = 0; pos < ARRAY_SIZE(chip->pin_bitmap); pos++)
			root_cell.arch.irq_bitmap[chip->pin_base / 32 + pos] |=
				chip->pin_bitmap[pos];

	/* mask out pins again that actually didn't belong to the root cell */
	for_each_irqchip(chip, root_cell.config, n)
	        for (pos = 0; pos < ARRAY_SIZE(chip->pin_bitmap); pos++)
			root_cell.arch.irq_bitmap[chip->pin_base / 32 + pos] &=
				chip->pin_bitmap[pos];
}

void irqchip_send_virq(struct cell *cell, unsigned int irq)
{
	struct public_per_cpu *pcpu;
	unsigned int cpu;

	//printk("PLIC: sending vIRQ %u from %s to %s\n", irq, this_cell()->config->name, cell->config->name);

	if (!irq_bitmap_test(cell->arch.virq_present_bitmap, irq)) {
		printk("vIRQ not present in destination\n");
		return;
	}

	// Do we need to lock this section? A vIRQ could be disabled during injection
	for_each_cpu(cpu, &cell->cpu_set) {
		pcpu = public_per_cpu(cpu);	
		if (irq_bitmap_test(pcpu->virq_enabled_bitmap, irq)) {
			irq_bitmap_set(pcpu->virq_pending_bitmap, irq);
			memory_barrier();
			arch_send_event(pcpu);
			break;
		}
	}
}

void irqchip_register_virq(unsigned int irq)
{
	struct cell *cell = this_cell();

	if (irqchip_irq_in_cell(cell, irq)) {
		printk("FATAL: plic: Unable to register vIRQ %u\n", irq);
		panic_stop();
	}

	irq_bitmap_set(cell->arch.virq_present_bitmap, irq);
}

void irqchip_unregister_virq(unsigned int irq)
{
	struct cell *cell = this_cell();
	unsigned int cpu;

	if (!irq_bitmap_test(cell->arch.virq_present_bitmap, irq))
		return;

	irq_bitmap_clear(cell->arch.virq_present_bitmap, irq);
	for_each_cpu(cpu, &cell->cpu_set)
		irq_bitmap_clear(public_per_cpu(cpu)->virq_enabled_bitmap, irq);
}

bool irqchip_inject_pending_virqs(void)
{
	struct public_per_cpu *pcpu = this_cpu_public();
	u32 idx, irq = 0;

	for (idx = 0; idx < ARRAY_SIZE(pcpu->virq_pending_bitmap); idx++) {
		irq = pcpu->virq_pending_bitmap[idx];
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
		irq_bitmap_clear(pcpu->virq_pending_bitmap, irq);
		return true;
	}

	return false;
}

void irqchip_process_pending_virqs(void)
{
	/*
	 * We can only inject IRQs if there's no other IRQ waiting. No problem:
	 * If other IRQs are currently being handled, the cell must somewhen
	 * acknowledge the interrupt. On acknowledgement, this routine is
	 * called again, so we won't miss the IRQ.
	 */
	if (guest_ext_pending())
		return;

	if (!irqchip_inject_pending_virqs())
		return;

	ext_disable();
	guest_inject_ext();
}

DEFINE_UNIT(irqchip, "RISC-V irqchip");
