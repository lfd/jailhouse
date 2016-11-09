/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2014-2016
 *
 * Authors:
 *  Henning Schild <henning.schild@siemens.com>
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_IVSHMEM_H
#define _JAILHOUSE_IVSHMEM_H

#include <jailhouse/pci.h>
#include <asm/apic.h>

#define IVSHMEM_CFG_MSIX_CAP	0x50
#define IVSHMEM_CFG_SIZE	(IVSHMEM_CFG_MSIX_CAP + 12)

/**
 * @defgroup PCI-IVSHMEM ivshmem
 * @{
 */

struct pci_ivshmem_endpoint {
	u32 cspace[IVSHMEM_CFG_SIZE / sizeof(u32)];
	u32 ivpos;
	u64 bar0_address;
	u64 bar4_address;
	struct pci_device *device;
	struct pci_ivshmem_endpoint *remote;
	struct apic_irq_message irq_msg;
};

int pci_ivshmem_init(struct cell *cell, struct pci_device *device);
void pci_ivshmem_exit(struct pci_device *device);
int pci_ivshmem_update_msix(struct pci_device *device);
enum pci_access pci_ivshmem_cfg_write(struct pci_device *device,
				      unsigned int row, u32 mask, u32 value);
enum pci_access pci_ivshmem_cfg_read(struct pci_device *device, u16 address,
				     u32 *value);

/** @} PCI-IVSHMEM */
#endif /* !_JAILHOUSE_IVSHMEM_H */
