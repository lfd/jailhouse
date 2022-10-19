/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/printk.h>
#include <jailhouse/entry.h>
#include <jailhouse/percpu.h>
#include <jailhouse/processor.h>
#include <jailhouse/utils.h>
#include <asm/control.h>
#include <asm/irqchip.h>
#include <asm/processor.h>
#include <asm/sbi.h>
#include <asm/setup.h>

void arch_handle_trap(union registers *regs);
void arch_handle_fault(union registers *regs);

static const char *causes[] = {
	[EXC_INST_MISALIGNED]		= "Instruction Address Misaligned",
	[EXC_INST_ACCESS]		= "Instruction Address Fault",
	[EXC_INST_ILLEGAL]		= "Illegal Instruction",
	[EXC_BREAKPOINT]		= "Breakpoint",
	[EXC_LOAD_ACCESS_MISALIGNED]	= "Load Accesss Misaligned",
	[EXC_LOAD_ACCESS]		= "Load Access Fault",
	[EXC_AMO_ADDRESS_MISALIGNED]	= "AMO Address Misaligned",
	[EXC_STORE_ACCESS]		= "Store Access Fault",
	[EXC_SYSCALL]			= "Env Call From U-Mode",
	[EXC_HYPERVISOR_SYSCALL]	= "Env Call From S-Mode",
	[EXC_SUPERVISOR_SYSCALL]	= "Env Call From VS-Mode",
	[11]				= "Env Call From M-Mode",
	[EXC_INST_PAGE_FAULT]		= "Instruction Pagefault",
	[EXC_LOAD_PAGE_FAULT]		= "Load Pagefault",
	[14]				= "Reserved",
	[EXC_STORE_PAGE_FAULT]		= "Store Pagefault",
	[16 ... 19]			= "Reserved",
	[EXC_INST_GUEST_PAGE_FAULT]	= "Inst Guest Pagefault",
	[EXC_LOAD_GUEST_PAGE_FAULT]	= "Load Guest Pagefault",
	[EXC_VIRTUAL_INST_FAULT]	= "Virtual Instruction Fault",
	[EXC_STORE_GUEST_PAGE_FAULT]	= "Store Guest Pagefault",
};

static inline bool is_irq(u64 cause)
{
	return !!(cause & CAUSE_IRQ_FLAG);
}

static inline unsigned long to_irq(unsigned long cause)
{
	return cause & ~CAUSE_IRQ_FLAG;
}

static void dump_regs(union registers *r)
{
	struct public_per_cpu *pc = this_cpu_public();
	unsigned long scause = csr_read(scause);
	const char *cause_str = "UNKNOWN";

	/* We might want to later exchange it with panic_printk, but for the
	 * moment of development, printk is more helpful. */
	void (*printer)(const char *, ...) = panic_printk;

	if (scause <= 23)
		cause_str = causes[scause];
	else if (is_irq(scause))
		cause_str = "IRQ";

	printer("Fatal: Exception on CPU %u (HART %u). Cause: %lu (%s)\n"
		"scause: 0x%016lx, stval: 0x%016lx, htval<<2: 0x%016lx\n"
		" PC: 0x%016llx RA: 0x%016llx  SP: 0x%016llx\n"
		" GP: 0x%016llx TP: 0x%016llx  T0: 0x%016llx\n"
		" T1: 0x%016llx T2: 0x%016llx  S0: 0x%016llx\n"
		" S1: 0x%016llx A0: 0x%016llx  A1: 0x%016llx\n"
		" A2: 0x%016llx A3: 0x%016llx  A4: 0x%016llx\n"
		" A5: 0x%016llx A6: 0x%016llx  A7: 0x%016llx\n"
		" S2: 0x%016llx S3: 0x%016llx  S4: 0x%016llx\n"
		" S5: 0x%016llx S6: 0x%016llx  S7: 0x%016llx\n"
		" S8: 0x%016llx S9: 0x%016llx S10: 0x%016llx\n"
		"S11: 0x%016llx T3: 0x%016llx  T4: 0x%016llx\n"
		" T5: 0x%016llx T6: 0x%016llx\n",
		pc->cpu_id, pc->phys_id, to_irq(scause), cause_str,
		scause, csr_read(stval), csr_read(CSR_HTVAL) << 2,
		r->pc, r->ra, r->sp,
		r->gp, r->tp, r->t0,
		r->t1, r->t2, r->s0,
		r->s1, r->a0, r->a1,
		r->a2, r->a3, r->a4,
		r->a5, r->a6, r->a7,
		r->s2, r->s3, r->s4,
		r->s5, r->s6, r->s7,
		r->s8, r->s9, r->s10,
		r->s11, r->t3, r->t4,
		r->t5, r->t6);
}

static inline int handle_timer(void)
{
	this_cpu_public()->stats[JAILHOUSE_CPU_STAT_VMEXITS_TIMER]++;

	/*
	 * This routine must not be called, if SSTC is available: Either the
	 * guest uses STCE on its own, or we catch the sbi_set_timer() call,
	 * and set the guest's vstimecmp register.
	 */
	if (has_sstc)
		return trace_error(-EINVAL);

	sbi_set_timer(-1);

	/* inject timer to VS */
	csr_set(CSR_HVIP, VIE_TIE);

	return 0;
}

static inline void riscv_guest_inject_ipi(void)
{
	/* inject IPI to VS */
	csr_set(CSR_HVIP, VIE_SIE);
}

static inline int handle_ipi(void)
{
	struct public_per_cpu *pcpu = this_cpu_public();
	bool check_events = false;
	u32 *stats = pcpu->stats;
	int err = 0;

	/* Take the control lock */
	spin_lock(&pcpu->control_lock);

	/*
	 * Next, we have to check who's the recipient of the IPI. Is it the
	 * hypervisor, or is it the guest?
	 */
	switch (pcpu->ipi_cause) {
		case IPI_CAUSE_GUEST:
			stats[JAILHOUSE_CPU_STAT_VMEXITS_IPI]++;
			riscv_guest_inject_ipi();
			break;

		case IPI_CAUSE_MGMT:
			stats[JAILHOUSE_CPU_STAT_VMEXITS_MANAGEMENT]++;
			check_events = true;
			break;

		case IPI_CAUSE_NONE:
		default:
			printk("Fatal: Invalid IPI cause on CPU %u\n",
			       this_cpu_id());
			err = -EINVAL;
			break;
	}

	this_cpu_public()->ipi_cause = IPI_CAUSE_NONE;
	/*
	 * IPI must be acknowledged. Clear the IPI by clearing sip.SSIP.
	 * Alternatively, IPI could be cleared via SBI. This is deprecated and
	 * unperformant.
	 */
	csr_clear(sip, (1 << IRQ_S_SOFT));

	/*
	 * Unlock the control lock. Another CPU may us now send some further
	 * events, until we enter arch_check_events. It is important that the
	 * IPI is acknowledged here, as from now on, further IPIs might already
	 * be sent by remote CPUs.
	 */
	spin_unlock(&pcpu->control_lock);

	if (check_events)
		arch_check_events();

	return err;
}

static int handle_irq(u64 irq)
{
	int err;

	switch (irq) {
		case IRQ_S_TIMER:
			err = handle_timer();
			break;

		case IRQ_S_SOFT:
			err = handle_ipi();
			break;

		case IRQ_S_EXT:
			err = irqchip_set_pending();
			break;

		default:
			err = -ENOSYS;
			break;
	}

	return err;
}

static inline int sbi_ext_time(struct sbiret *ret, unsigned int fid, u64 stime)
{
	if (fid != SBI_EXT_TIME_SET_TIMER)
		return -ENOSYS;

	if (has_sstc) {
		ret->error = SBI_SUCCESS;
		csr_write(CSR_VSTIMECMP, stime);
		return 0;
	}

	/* Clear pending IRQs */
	csr_clear(CSR_HVIP, VIE_TIE);

	*ret = sbi_set_timer(stime);

	return 0;
}

static inline void
skip_emulated_instruction(union registers *regs, unsigned int b)
{
	regs->pc += b;
	csr_write(sepc, regs->pc);
}

static bool sbi_permitted_target_mask(unsigned long mask, unsigned long base)
{
	unsigned int cpu;
	unsigned long phys;

	for_each_cpu(cpu, &this_cell()->cpu_set) {
		phys = public_per_cpu(cpu)->phys_id;
		if (phys < base)
			continue;

		mask &= ~(1UL << (phys - base));
		if (!mask)
			return true;
	}

	return false;
}

static bool sbi_permitted_hart(unsigned long hartid)
{
	unsigned int cpu;

	for_each_cpu(cpu, &this_cell()->cpu_set)
		if (public_per_cpu(cpu)->phys_id == hartid)
			return true;
	return false;
}

static void guest_queue_ipi(unsigned int cpu)
{
	struct public_per_cpu *pcpu = public_per_cpu(cpu);

retry:
	spin_lock(&pcpu->control_lock);
	if (pcpu->ipi_cause == IPI_CAUSE_NONE)
		pcpu->ipi_cause = IPI_CAUSE_GUEST;
	else {
		spin_unlock(&pcpu->control_lock);
		goto retry;
	}
	spin_unlock(&pcpu->control_lock);
}

static inline int
handle_sbi_send_ipi(struct sbiret *ret, unsigned long mask, unsigned long base)
{
	unsigned long phys, mask_fwd;
	unsigned int cpu;

	mask_fwd = 0;
	while (mask) {
		phys = ffsl(mask);
		mask &= ~(1UL << phys);

		cpu = cpu_by_phys_processor_id(base + phys);
		if (cpu == this_cpu_id())
			riscv_guest_inject_ipi();
		else if (public_per_cpu(cpu)->cell != this_cell())
			return -EPERM;
		else {
			guest_queue_ipi(cpu);
			mask_fwd |= (1UL << phys);
		}
	}

	/* Only forward the IPI, if in mask is anything left */
	if (mask_fwd)
		*ret = sbi_send_ipi(mask_fwd, base);
	else /* Just a single self-IPI */
		ret->value = ret->error = 0;

	return 0;
}


static inline int handle_sbi_rfence(struct sbiret *ret, unsigned int fid,
				    unsigned long mask, unsigned long base,
				    unsigned long a2, unsigned long a3,
				    unsigned long a4)
{
	if (!sbi_permitted_target_mask(mask, base))
			return -EPERM;

	*ret = sbi_ecall(SBI_EXT_RFENCE, fid, mask, base, a2, a3, a4, 0);
	return 0;
}

static int riscv_unpark(struct sbiret *ret, unsigned long hartid,
			unsigned long start_addr, unsigned long opaque)
{
	struct public_per_cpu *pcpu;
	unsigned int cpu;
	int err = 0;

	cpu = cpu_by_phys_processor_id(hartid);
	pcpu = public_per_cpu(cpu);
	ret->value = 0;

	spin_lock(&pcpu->control_lock);
	if (pcpu->hsm.state == STARTED || pcpu->hsm.state == START_PENDING) {
		ret->error = SBI_ERR_ALREADY_AVAILABLE;
		goto unlock_out;
	}

	pcpu->hsm.start_addr = start_addr;
	pcpu->hsm.opaque = opaque;
	pcpu->hsm.state = START_PENDING;
	spin_unlock(&pcpu->control_lock);

	arch_send_event(pcpu);

	ret->error = SBI_SUCCESS;

unlock_out:
	spin_unlock(&pcpu->control_lock);
	return err;
}

static inline unsigned long hsm_state(unsigned long hart)
{
	return public_per_cpu(cpu_by_phys_processor_id(hart))->hsm.state;
}

static inline int
handle_sbi_hsm(struct sbiret *ret, unsigned int fid, unsigned long a0,
	       unsigned long a1, unsigned long a2)
{
	int err = 0;

	switch (fid) {
		case SBI_EXT_HSM_HART_STOP:
			riscv_park_cpu();
			break;

		case SBI_EXT_HSM_HART_START:
			ret->value = 0;
			if (!sbi_permitted_hart(a0) ||
			    a0 == this_cpu_public()->phys_id) {
				ret->error = SBI_ERR_INVALID_PARAM;
				break;
			}

			err = riscv_unpark(ret, a0, a1, a2);
			break;

		case SBI_EXT_HSM_HART_STATUS:
			if (sbi_permitted_hart(a0)) {
				ret->error = SBI_SUCCESS;
				ret->value = hsm_state(a0);
			} else {
				ret->error = SBI_ERR_INVALID_PARAM;
				ret->value = 0;
			}
			break;

		default:
			printk("Unknown HSM Fid: %u\n", fid);
			err = -EINVAL;
			break;
	}

	return err;
}

static struct sbiret sbi_probe_ext(unsigned long ext)
{
	struct sbiret ret;

	/* Allow access to all extensions but system reset (SRST) */
	switch (ext) {
	case SBI_EXT_TIME:
	case SBI_EXT_SPI:
	case SBI_EXT_RFENCE:
	case SBI_EXT_HSM:
		ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_PROBE_EXT, ext,
				0, 0, 0, 0, 0);
		break;

	case SBI_EXT_SRST:
	default:
		ret.error = SBI_ERR_DENIED;
		ret.value = 0;
		break;
	}

	return ret;
}

static inline int
sbi_ext_base(struct sbiret *ret, unsigned long fid, unsigned long a0)
{
	int err = 0;

	switch (fid) {
		case SBI_EXT_BASE_GET_SPEC_VERSION:
		case SBI_EXT_BASE_GET_IMP_ID:
		case SBI_EXT_BASE_GET_IMP_VERSION:
		case SBI_EXT_BASE_GET_MVENDORID:
		case SBI_EXT_BASE_GET_MARCHID:
		case SBI_EXT_BASE_GET_MIMPID:
			*ret = sbi_ecall(SBI_EXT_BASE, fid, a0, 0, 0, 0, 0, 0);
			break;

		case SBI_EXT_BASE_PROBE_EXT:
			*ret = sbi_probe_ext(a0);
			break;

		default:
			err = -ENOSYS;
			break;
	}

	return err;
}

static int handle_ecall(union registers *regs)
{
	/*
	 * Spec: In the name of compatibility, SBI extension IDs (EIDs) and SBI
	 * function IDs (FIDs) are encoded as signed 32-bit integers. When
	 * passed in registers these follow the standard above calling
	 * convention rules.
	 */
	u32 *stats = this_cpu_public()->stats;
	unsigned int eid, fid;
	struct sbiret ret;
	int err = -ENOSYS;

	eid = regs->a7;
	fid = regs->a6;
	ret.value = 0;

	switch (eid) {
		/* Treat putchar like a hypercall. Accounts for a hypercall. */
		case SBI_EXT_0_1_CONSOLE_PUTCHAR:
			ret.error = hypercall(JAILHOUSE_HC_DEBUG_CONSOLE_PUTC,
					      regs->a0, 0);
			err = 0;
			break;

		case SBI_EXT_0_1_CONSOLE_GETCHAR:
			stats[JAILHOUSE_CPU_STAT_VMEXITS_SBI_OTHER]++;
			/*
			 * I have never seen this one being used in real-world.
			 * As we would need to think if we allow access (what
			 * about non-root?), let's simply deny it for the
			 * moment.
			 */
			if (0)
				ret = sbi_console_getchar_legacy_0_1();
			else {
				ret.error = SBI_ERR_DENIED;
				ret.value = 0;
			}
			err = 0;
			break;

		case SBI_EXT_BASE:
			stats[JAILHOUSE_CPU_STAT_VMEXITS_SBI_OTHER]++;
			err = sbi_ext_base(&ret, fid, regs->a0);
			break;

		case SBI_EXT_TIME: /* since SBI v0.2 */
			stats[JAILHOUSE_CPU_STAT_VMEXITS_SBI_TIME]++;
			err = sbi_ext_time(&ret, fid, regs->a0);
			break;

		case SBI_EXT_SPI:
			stats[JAILHOUSE_CPU_STAT_VMEXITS_SBI_IPI]++;
			err = handle_sbi_send_ipi(&ret, regs->a0, regs->a1);
			break;

		case SBI_EXT_RFENCE:
			stats[JAILHOUSE_CPU_STAT_VMEXITS_SBI_RFENCE]++;
			err = handle_sbi_rfence(&ret, fid, regs->a0, regs->a1,
						regs->a2, regs->a3, regs->a4);
			break;

		case SBI_EXT_HSM:
			stats[JAILHOUSE_CPU_STAT_VMEXITS_SBI_OTHER]++;
			err = handle_sbi_hsm(&ret, fid, regs->a0, regs->a1,
					     regs->a2);
			break;

		case JAILHOUSE_EID:
			ret.error = hypercall(fid, regs->a0, regs->a1);
			if (fid == JAILHOUSE_HC_DISABLE && !ret.error)
				riscv_deactivate_vmm(regs, 0, true);
			err = 0;
			break;

		default:
			printk("Unknown SBI call EID: 0x%x FID: 0x%x\n",
			       eid, fid);
			return -EINVAL;
			break;
	}

	if (err)
		return err;

	/* If we came from stop, don't propagate error codes */
	if (eid == SBI_EXT_HSM && fid == SBI_EXT_HSM_HART_STOP)
		return 0;

	regs->a0 = ret.error;
	regs->a1 = ret.value;
	skip_emulated_instruction(regs, 4);

	return 0;
}

static inline u16 gmem_read16(unsigned long addr)
{
	u64 mem;

	/*
	 * hlvx.hu can potentially fault and throw an exception. But if we end
	 * up here, we're decoding an instruction that the guest was possible
	 * to execute. Hence, it must be backed by existing memory, and no
	 * exception can occur.
	 */
	asm volatile(".insn r 0x73, 0x4, 0x32, %0, %1, x3\n" /* hlvx.hu */
		     : "=r"(mem) : "r"(addr) : "memory");

	return mem;
}

static inline u64 sext(u32 w)
{
	u64 ret;
	asm volatile("sext.w %0, %0" : "=r"(ret) : "r"(w) :);
	return ret;
}

#define COMP_RX_OFF	8

union instruction {
	struct {
		u32 type:2;
		u32 opcode:5;
		u32 dest:5;
		u32 width:3;
		u32 base:5;
		u32 offset:12;
	} load __attribute__((packed));
	struct {
		u32 type:2;
		u32 opcode:5;
		u32 offset40:5;
		u32 width:3;
		u32 base:5;
		u32 src:5;
		u32 offset115:7;
	} store __attribute__((packed));
	struct {
		u32 type:2;
		u32 opcode:5;
		u32 rsvd:25;
	} generic __attribute__((packed));
	u32 raw;
} __attribute__((packed));

union cinstruction {
	struct {
		u16 opcode:2;
		u16 src_dst:3;
		u16 off1:2;
		u16 base:3;
		u16 off2:3;
		u16 funct3:3;
	} load_store __attribute__((packed));
	struct {
		u16 opcode:2;
		u16 rsvd:14;
	} generic __attribute__((packed));
	u16 raw;
} __attribute__((packed));

struct riscv_mmio_inst {
	unsigned char reg;
	unsigned char width;
	bool sign_extended;
};

static int
riscv_decode_compressed_instruction(struct riscv_mmio_inst *mmio_inst, u16 inst)
{
	union cinstruction i;
	int err = -ENOSYS;

	i.raw = inst;
	/* SW */
	if (i.generic.opcode == 0 && i.load_store.funct3 == 6) {
		mmio_inst->width = 4;
		mmio_inst->sign_extended = true;
		mmio_inst->reg = i.load_store.src_dst + COMP_RX_OFF;
		err = 0;
	/* LW */
	} else if (i.generic.opcode == 0 && i.load_store.funct3 == 2) {
		mmio_inst->width = 4;
		mmio_inst->sign_extended = true;
		mmio_inst->reg = i.load_store.src_dst + COMP_RX_OFF;
		err = 0;
	}

	return err;
}

static int riscv_decode_instruction(struct riscv_mmio_inst *mmio_inst, u32 inst,
				    bool is_compressed)
{
	union instruction i;
	int err = -ENOSYS;

	if (is_compressed)
		return riscv_decode_compressed_instruction(mmio_inst, inst);

	i.raw = inst;
	if (i.generic.type != 0x3)
		return err;

	/* LB, LH, LW */
	if (i.generic.opcode == 0x0) {
		if (i.load.width > 2)
			return err;
		mmio_inst->width = 1 << i.load.width;
		mmio_inst->sign_extended = true;
		mmio_inst->reg = i.load.dest;
		err = 0;
	/* SB, SW, SH */
	} else if (i.generic.opcode == 0x8) {
		if (i.store.width > 2)
			return err;
		mmio_inst->width = 1 << i.store.width;
		mmio_inst->sign_extended = true;
		mmio_inst->reg = i.store.src;
		err = 0;
	}

	return err;
}

static int handle_fault(union registers *regs, bool is_write)
{
	struct riscv_mmio_inst mmio_inst;
	struct mmio_access mmio;
	enum mmio_result result;
	bool is_compressed;
	u32 instruction;
	size_t *reg;
	int err;

	this_cpu_public()->stats[JAILHOUSE_CPU_STAT_VMEXITS_MMIO]++;

	mmio.is_write = is_write;
	mmio.address = csr_read(CSR_HTVAL) << 2;

	/*
	 * Potentially, htval might point to address NULL, while pointing to a
	 * valid trap value. However, 0 might also indicate that htval is not
	 * supported by the micro-architecture. Hence, by design, let's say
	 * that address NULL should (a) not be used or (b) must not cause
	 * access faults when being used.
	 *
	 * Here, we assume that the micro-architecture doesn't support htval,
	 * if we read back zero on a fault exception.
	 */
	if (!mmio.address)
		return -ENOSYS;

	/* Just ensure that the instruction is 16-bit aligned */
	if (regs->pc & 0x1)
		return -EINVAL;

	/* Load remaining lsb two bits from stval */
	mmio.address |= csr_read(stval) & 0x3;

	/* Load faulting instruction */

	/* check if htinst is available */
#if 0 /* htinst can hold pseudo-decoded instrs which we don't support yet. */
	instruction = csr_read(CSR_HTINST);
#else
	instruction = 0;
#endif
	if (instruction != 0) {
		is_compressed = (instruction & 0x3) != 0x3;
	} else { /* if not, load from guest mem */
		instruction = gmem_read16(regs->pc);
		if ((instruction & 0x3) == 0x3) {
			is_compressed = false;
			instruction |= (u32)gmem_read16(regs->pc + 2) << 16;
		} else
			is_compressed = true;
	}

	err = riscv_decode_instruction(&mmio_inst, instruction, is_compressed);
	if (err)
		goto unsup;

	if (mmio.is_write)
		mmio.value = regs->raw.x[mmio_inst.reg];
	mmio.size = mmio_inst.width;

	result = mmio_handle_access(&mmio);
	if (result == MMIO_HANDLED) {
		if (!mmio.is_write) {
			reg = &regs->raw.x[mmio_inst.reg];
			*reg = mmio.value;
			if (mmio_inst.width < 8) {
				*reg &= ((1UL << mmio_inst.width * 8) - 1);
				if (mmio_inst.sign_extended)
					*reg = sext(*reg);
			}
		}
		skip_emulated_instruction(regs, is_compressed ? 2 : 4);
		return 0;
	}

	return -ENOSYS;

unsup:
	printk("Unsupported instruction: 0x%x\n", instruction);
	return -ENOSYS;
}

void arch_handle_fault(union registers *regs)
{
	panic_printk("FATAL: Unhandled S-Mode exception.\n");
	panic_printk("Hypervisor registers:\n");
	dump_regs(regs);
	panic_stop();
}

void arch_handle_trap(union registers *regs)
{
	unsigned long cause;
	int err = -1;

	this_cpu_public()->stats[JAILHOUSE_CPU_STAT_VMEXITS_TOTAL]++;

	regs->pc = csr_read(sepc);
	cause = csr_read(scause);

	if (is_irq(cause)) {
		err = handle_irq(cause & ~CAUSE_IRQ_FLAG);
		goto out;
	}

	switch (cause) {
		case EXC_INST_ACCESS:
		case EXC_LOAD_ACCESS:
		case EXC_STORE_ACCESS:
		case EXC_INST_PAGE_FAULT:
		case EXC_LOAD_PAGE_FAULT:
		case EXC_STORE_PAGE_FAULT:
		case EXC_INST_MISALIGNED:
		case EXC_LOAD_ACCESS_MISALIGNED:
		case EXC_AMO_ADDRESS_MISALIGNED:
			printk("\nFaulting Address: %016lx\n", csr_read(stval));
			err = -ENOSYS;
			break;

		case EXC_SUPERVISOR_SYSCALL:
			err = handle_ecall(regs);
			break;

		case EXC_BREAKPOINT:
			printk("BP occured @ PC: %016lx\n", regs->pc);
			err = -1;
			break;

		case EXC_LOAD_GUEST_PAGE_FAULT:
			err = handle_fault(regs, false);
			break;

		case EXC_STORE_GUEST_PAGE_FAULT:
			err = handle_fault(regs, true);
			break;

		case EXC_INST_ILLEGAL:
		default:
			err = -1;
			break;
	}

out:
	if (err) {
		dump_regs(regs);
		panic_park();
	}
}
