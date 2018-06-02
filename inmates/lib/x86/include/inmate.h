/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013-2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _JAILHOUSE_INMATE_H
#define _JAILHOUSE_INMATE_H

#include <asm-generic/types.h>

#define INMATE_CS32		0x8
#define INMATE_CS64		0x10
#define INMATE_DS32		0x18

#define APIC_LVL_ASSERT		(1 << 14)

#define PCI_CFG_VENDOR_ID	0x000
#define PCI_CFG_DEVICE_ID	0x002
#define PCI_CFG_COMMAND		0x004
# define PCI_CMD_IO		(1 << 0)
# define PCI_CMD_MEM		(1 << 1)
# define PCI_CMD_MASTER		(1 << 2)
# define PCI_CMD_INTX_OFF	(1 << 10)
#define PCI_CFG_STATUS		0x006
# define PCI_STS_INT		(1 << 3)
# define PCI_STS_CAPS		(1 << 4)
#define PCI_CFG_BAR		0x010
# define PCI_BAR_64BIT		0x4
#define PCI_CFG_CAP_PTR		0x034

#define PCI_ID_ANY		0xffff

#define PCI_DEV_CLASS_OTHER	0xff

#define PCI_CAP_MSI		0x05
#define PCI_CAP_MSIX		0x11

#define MSIX_CTRL_ENABLE	0x8000
#define MSIX_CTRL_FMASK		0x4000

#define SMP_MAX_CPUS		255

#ifndef __ASSEMBLY__

typedef void(*int_handler_t)(void);

void int_init(void);
void int_set_handler(unsigned int vector, int_handler_t handler);
void int_send_ipi(unsigned int cpu_id, unsigned int vector);

enum ioapic_trigger_mode {
	TRIGGER_EDGE = 0,
	TRIGGER_LEVEL_ACTIVE_HIGH = 1 << 15,
	TRIGGER_LEVEL_ACTIVE_LOW = (1 << 15) | (1 << 13),
};

void ioapic_init(void);
void ioapic_pin_set_vector(unsigned int pin,
			   enum ioapic_trigger_mode trigger_mode,
			   unsigned int vector);

void hypercall_init(void);

unsigned long pm_timer_read(void);

unsigned long tsc_read(void);
unsigned long tsc_init(void);

void delay_us(unsigned long microsecs);

unsigned long apic_timer_init(unsigned int vector);
void apic_timer_set(unsigned long timeout_ns);

u32 pci_read_config(u16 bdf, unsigned int addr, unsigned int size);
void pci_write_config(u16 bdf, unsigned int addr, u32 value,
		      unsigned int size);
int pci_find_device(u16 vendor, u16 device, u16 start_bdf);
int pci_find_cap(u16 bdf, u16 cap);
void pci_msi_set_vector(u16 bdf, unsigned int vector);
void pci_msix_set_vector(u16 bdf, unsigned int vector, u32 index);

extern volatile u32 smp_num_cpus;
extern u8 smp_cpu_ids[SMP_MAX_CPUS];
void smp_wait_for_all_cpus(void);
void smp_start_cpu(unsigned int cpu_id, void (*entry)(void));
#endif

#include <inmate_common.h>

#endif /* !_JAILHOUSE_INMATE_H */
