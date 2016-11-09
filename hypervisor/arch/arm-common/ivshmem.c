/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2016
 *
 * Author:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/ivshmem.h>
#include <asm/irqchip.h>

void arch_ivshmem_write_doorbell(struct ivshmem_endpoint *ive)
{
	struct ivshmem_endpoint *remote = ive->remote;
	unsigned int irq_id;

	if (!remote)
		return;

	irq_id = remote->arch.irq_id;
	if (irq_id)
		irqchip_set_pending(NULL, irq_id);
}

int arch_ivshmem_update_msix(struct pci_device *device)
{
	struct ivshmem_endpoint *ive = device->ivshmem_endpoint;
	unsigned int irq_id = 0;

	if (!ivshmem_is_msix_masked(ive)) {
		/* FIXME: validate MSI-X target address */
		irq_id = device->msix_vectors[0].data;
		if (!irqchip_irq_in_cell(device->cell, irq_id))
			return -EPERM;
	}

	ive->arch.irq_id = irq_id;

	return 0;
}

void arch_ivshmem_init(struct ivshmem_endpoint *ive, struct cell *cell)
{
	if (ive->device->info->num_msix_vectors == 0) {
		u8 pin = ive->cspace[PCI_CFG_INT/4] >> 8;

		ive->arch.irq_id = cell->config->vpci_irq_base + pin - 1;
	}
}
