/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Copyright (C) 2015 Regents of the University of California
 */

/* This file primarily bases upon definitions from the Linux Kernel */

#ifdef __ASSEMBLY__
#define _AC(X,Y)	X
#define _AT(T,X)	X
#else
#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _AT(T,X)	((T)(X))
#endif

#define CSR_SATP	0x180
#define CSR_VSSTATUS	0x200
#define CSR_VSIE	0x204
#define CSR_VSTVEC	0x205
#define CSR_VSSCRATCH	0x240
#define CSR_VSEPC	0x241
#define CSR_VSCAUSE	0x242
#define CSR_VSTVAL	0x243
#define CSR_VSATP	0x280
#define CSR_HSTATUS	0x600
#define CSR_HEDELEG	0x602
#define CSR_HIDELEG	0x603
#define CSR_HIE		0x604
#define CSR_HTIMEDELTA	0x605
#define CSR_HCOUNTEREN	0x606
#define CSR_HGEIE	0x607
#define CSR_HTVAL	0x643
#define CSR_HIP		0x644
#define CSR_HVIP	0x645
#define CSR_HTINST	0x64a
#define CSR_HGATP	0x680

/* Status register flags */
#define SR_SIE		_AC(0x00000002, UL) /* Supervisor Interrupt Enable */
#define SR_MIE		_AC(0x00000008, UL) /* Machine Interrupt Enable */
#define SR_SPIE		_AC(0x00000020, UL) /* Previous Supervisor IE */
#define SR_MPIE		_AC(0x00000080, UL) /* Previous Machine IE */
#define SR_SPP		_AC(0x00000100, UL) /* Previously Supervisor */
#define SR_MPP          _AC(0x00001800, UL) /* Previously Machine */
#define SR_SUM		_AC(0x00040000, UL) /* Supervisor User Memory Access */

#define SR_FS		_AC(0x00006000, UL) /* Floating-point Status */
#define SR_FS_OFF	_AC(0x00000000, UL)
#define SR_FS_INITIAL	_AC(0x00002000, UL)
#define SR_FS_CLEAN	_AC(0x00004000, UL)
#define SR_FS_DIRTY	_AC(0x00006000, UL)

#define SR_XS		_AC(0x00018000, UL) /* Extension Status */
#define SR_XS_OFF	_AC(0x00000000, UL)
#define SR_XS_INITIAL	_AC(0x00008000, UL)
#define SR_XS_CLEAN	_AC(0x00010000, UL)
#define SR_XS_DIRTY	_AC(0x00018000, UL)

#define SR_VS_DIRTY	_AC(0x00000600, UL)

#define SR_UXL_64	_AC(0x200000000, UL)

#define SR_SD		_AC(0x8000000000000000, UL) /* FS/XS dirty */

/* Exception causes */
#define EXC_INST_MISALIGNED		0
#define EXC_INST_ACCESS			1
#define EXC_INST_ILLEGAL		2
#define EXC_BREAKPOINT			3
#define EXC_LOAD_ACCESS_MISALIGNED	4
#define EXC_LOAD_ACCESS			5
#define EXC_AMO_ADDRESS_MISALIGNED	6
#define EXC_STORE_ACCESS		7
#define EXC_SYSCALL			8
#define EXC_HYPERVISOR_SYSCALL		9
#define EXC_SUPERVISOR_SYSCALL		10
#define EXC_INST_PAGE_FAULT		12
#define EXC_LOAD_PAGE_FAULT		13
#define EXC_STORE_PAGE_FAULT		15
#define EXC_INST_GUEST_PAGE_FAULT	20
#define EXC_LOAD_GUEST_PAGE_FAULT	21
#define EXC_VIRTUAL_INST_FAULT		22
#define EXC_STORE_GUEST_PAGE_FAULT	23

/* Exception cause high bit - is an interrupt if set */
#define CAUSE_IRQ_FLAG	(_AC(1, UL) << (__riscv_xlen - 1))

/* Interrupt causes (minus the high bit) */
#define IRQ_S_SOFT		1
#define IRQ_VS_SOFT		2
#define IRQ_M_SOFT		3
#define IRQ_S_TIMER		5
#define IRQ_VS_TIMER		6
#define IRQ_M_TIMER		7
#define IRQ_S_EXT		9
#define IRQ_VS_EXT		10
#define IRQ_M_EXT		11

/* VSIP & HVIP relation */
#define VSIP_TO_HVIP_SHIFT	(IRQ_VS_SOFT - IRQ_S_SOFT)

/* IE/IP (Supervisor/Machine Interrupt Enable/Pending) flags */
#define IE_SIE		(_AC(0x1, UL) << IRQ_S_SOFT)
#define IE_TIE		(_AC(0x1, UL) << IRQ_S_TIMER)
#define IE_EIE		(_AC(0x1, UL) << IRQ_S_EXT)

#define VIE_SIE		(IE_SIE << VSIP_TO_HVIP_SHIFT)
#define VIE_TIE		(IE_TIE << VSIP_TO_HVIP_SHIFT)
#define VIE_EIE		(IE_EIE << VSIP_TO_HVIP_SHIFT)

/* SATP flags */
#define SATP_PPN	_AC(0x00000FFFFFFFFFFF, UL)
#define SATP_MODE_39	_AC(0x8000000000000000, UL)
#define SATP_MODE_48	_AC(0x9000000000000000, UL)
#define SATP_ASID_BITS	16
#define SATP_ASID_SHIFT	44
#define SATP_ASID_MASK	_AC(0xFFFF, UL)

/* HSTATUS flags */
#define HSTATUS_VSXL            _AC(0x300000000, UL)
#define HSTATUS_VSXL_SHIFT      32

#define HSTATUS_VTSR            _AC(0x00400000, UL)
#define HSTATUS_VTW             _AC(0x00200000, UL)
#define HSTATUS_VTVM            _AC(0x00100000, UL)
#define HSTATUS_VGEIN           _AC(0x0003f000, UL)
#define HSTATUS_VGEIN_SHIFT     12
#define HSTATUS_HU              _AC(0x00000200, UL)
#define HSTATUS_SPVP            _AC(0x00000100, UL)
#define HSTATUS_SPV             _AC(0x00000080, UL)
#define HSTATUS_GVA             _AC(0x00000040, UL)
#define HSTATUS_VSBE            _AC(0x00000020, UL)

#define HGATP_VMID_SHIFT	22
#define HGATP_VMID_WIDTH	7

#define SCOUNTEREN_CY		0x00000001
#define SCOUNTEREN_TM		0x00000002

#ifndef __ASSEMBLY__

#include <jailhouse/string.h>

#define csr_from_csr(dst, src)					\
({								\
	register unsigned long __tmp;				\
	__asm__ __volatile__("csrr %0, " __stringify(src) "\n"	\
			     "csrw " __stringify(dst) ", %0\n"	\
			     : "=r"(__tmp) :			\
			     : "memory"				\
			    );					\
})

#define csr_read(csr)                                           \
({                                                              \
	register unsigned long __v;                             \
	__asm__ __volatile__ ("csrr %0, " __stringify(csr)      \
			      : "=r" (__v) :                    \
			      : "memory");                      \
	__v;                                                    \
})

#define csr_write(csr, val)                                     \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrw " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#define csr_clear(csr, val)                                     \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrc " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#define csr_set(csr, val)                                       \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrs " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#define csr_swap(csr, val)						\
({									\
	unsigned long __v = (unsigned long)(val);			\
	__asm__ __volatile__ ("csrrw %0, " __stringify(csr) ", %1"	\
			      : "=r" (__v) : "rK" (__v)			\
			      : "memory");				\
	__v;								\
})

#endif /* __ASSEMBLY__ */
