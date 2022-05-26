/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *  Stefan Huber <stefan.huber@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/cell.h>
#include <jailhouse/percpu.h>
#include <jailhouse/control.h>
#include <jailhouse/mmio.h>
#include <jailhouse/pci.h>

u32 arch_pci_read_config(u16 bdf, u16 address, unsigned int size)
{
	return 0;
}

void arch_pci_write_config(u16 bdf, u16 address, u32 value, unsigned int size)
{
}

int arch_pci_add_physical_device(struct cell *cell, struct pci_device *device)
{
	return 0;
}

void arch_pci_remove_physical_device(struct pci_device *device)
{
}

void arch_pci_set_suppress_msi(struct pci_device *device,
			       const struct jailhouse_pci_capability *cap,
			       bool suppress)
{
}

int arch_pci_update_msi(struct pci_device *device,
			const struct jailhouse_pci_capability *cap)
{
	const struct jailhouse_pci_device *info = device->info;
	union pci_msi_registers target;
	struct public_per_cpu *ppc;
	unsigned short vs_file;
	unsigned int cpu;
	unsigned int n;

	/* If the MSI is masked, allow to write any address */
	target = device->msi_registers;
	if (!device->msi_registers.msg32.enable)
		goto passthru;

	/* Only allow non-masked access, if vs-file is set */
	vs_file = this_cell()->arch.vs_file;
	if (!vs_file)
		return -EINVAL;

	for_each_cpu(cpu, &this_cell()->cpu_set) {
		ppc = public_per_cpu(cpu);
		/*
		 * If the MSI is unmasked, only allow if the address is
		 * on the S-Mode file and calculate the VS-mode offset.
		 */
		if (ppc->imsic_base == device->msi_registers.msg64.address) {
			target.msg64.address += vs_file * 0x1000;
			goto passthru;
		}
	}

	return -EINVAL;

passthru:
	for (n = 1; n < (info->msi_64bits ? 4 : 3); n++)
		pci_write_config(info->bdf, cap->start + n * 4,
				 target.raw[n], 4);

	return 0;
}

int arch_pci_update_msix_vector(struct pci_device *device, unsigned int index)
{
	struct public_per_cpu *ppc;
	unsigned short vs_file;
	unsigned int cpu;
	u64 vs_offset = 0;

	vs_file = this_cell()->arch.vs_file;

	/* If the MSI is masked, allow to write any address */
	if (device->msix_vectors[index].masked)
		goto passthru;

	/* Only allow non-masked access, if vs-file is set */
	if (!vs_file)
		return -EINVAL;

	/*
	 * If the MSI is unmasked, only allow if the address is on the S-Mode
	 * file and calculate the VS-mode offset.
	 */
	for_each_cpu(cpu, &this_cell()->cpu_set) {
		ppc = public_per_cpu(cpu);
		if (ppc->imsic_base == device->msix_vectors[index].address) {
			vs_offset = vs_file * 0x1000;
			goto passthru;
		}
	}

	return -EINVAL;

passthru:
	mmio_write64_split(&device->msix_table[index].address,
			   device->msix_vectors[index].address + vs_offset);
	mmio_write32(&device->msix_table[index].data,
		     device->msix_vectors[index].data);
	return 0;
}
