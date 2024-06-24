/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2016-2019
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/ivshmem.h>
#include <jailhouse/cell.h>
#include <asm/processor.h>
#include <asm/irqchip.h>

#include <jailhouse/printk.h>

void arch_ivshmem_trigger_interrupt(struct ivshmem_endpoint *ive,
				    unsigned int vector)
{
	unsigned int irq_id = ive->irq_cache.id[vector];

	printk("cpu %u: trigger vector %u (irq_id: %u)\n", this_cpu_id(), vector, irq_id);

	/* If we have an IMSIC, then always deliver the IRQ as MSI-X */
	if (imsic) {
		/*
		 * If a cell is shutdown, the device might already be lost.
		 * Further, heck for a non-zero irq id
		 */
		if (!ive->device || !irq_id)
			return;

		if (ive->device->msix_vectors[vector].masked)
			return;

		printk("writing to imsic..\n");
		imsic_write(
			    ive->device->msix_vectors[vector].address - imsic_base(),
			    ive->device->cell->arch.vs_file,
			    irq_id);
	} else if (irq_id) {
		/*
		 * Legacy IRQs - even if translated to MSIs - are delivered via
		 * the irqchip
		 */

		/*
		 * Ensure that all data written by the sending guest is visible
		 * to the target before triggering the interrupt.
		 */
		memory_barrier();

		irqchip_send_virq(ive->device->cell, irq_id);
	}
}

int arch_ivshmem_update_msix(struct ivshmem_endpoint *ive, unsigned int vector,
			     bool enabled)
{
	struct pci_device *device = ive->device;
	unsigned int irq_id;

	spin_lock(&ive->irq_lock);
	irq_id = device->msix_vectors[vector].data;
	ive->irq_cache.id[vector] = enabled ? irq_id : 0;
	spin_unlock(&ive->irq_lock);

	printk("cpu %u: update msix. vector: %u irq_id: %u enabled: %u\n", this_cpu_id(), vector, irq_id, enabled ? 1:0);

	return 0;
}

void arch_ivshmem_update_intx(struct ivshmem_endpoint *ive, bool enabled)
{
	u8 pin = ive->cspace[PCI_CFG_INT/4] >> 8;
	struct pci_device *device = ive->device;
	unsigned int virq;

	/*
	 * Lock used as barrier, ensuring all interrupts triggered after return
	 * use the new setting.
	 */
	virq = device->cell->config->vpci_irq_base + pin - 1;
	spin_lock(&ive->irq_lock);
	if (enabled) {
		ive->irq_cache.id[0] = virq;
		irqchip_register_virq(virq);
	} else {
		ive->irq_cache.id[0] = 0;
		irqchip_unregister_virq(virq);
	}
	spin_unlock(&ive->irq_lock);
}
