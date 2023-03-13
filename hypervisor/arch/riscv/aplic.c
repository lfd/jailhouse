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

#define REGISTER(X) REG_RANGE((X), (X) + IRQCHIP_REG_SZ)

#define DOMAINCFG		0x0
#define  DOMAINCFG_DM		2
#define  DOMAINCFG_BE		0
#define  DOMAINCFG_IE		8
#define  DOMAINCFG_DEFAULT	((1 << 31) | (1 << DOMAINCFG_IE))

#define SOURCECFG_BASE 		0x4
#define  SOURCECFG_D		10
#define SOURCECFG_END 		0x1000

#define TARGET_BASE		0x3004
#define TARGET_HART_SHIFT	18
#define TARGET_GUEST_SHIFT	12
#define TARGET_GUEST_MSK	BIT_MASK(17, 12)
#define TARGET_PRIO_MSK		BIT_MASK(7, 0)
#define TARGET_EIID_MSK		BIT_MASK(10, 0)

#define IN_CLRIP_START		0x1d00
#define IN_CLRIP_END		(0x1d7c + IRQCHIP_REG_SZ)

#define SETIE_BASE		0x1e00
#define SETIENUM		0x1edc

#define CLRIE_START		0x1f00
#define CLRIE_END		(0x1f7c + IRQCHIP_REG_SZ)
#define CLRIENUM		0x1fdc

#define SETIPNUM_LE		0x2000

/* Per-Hart Interrupt Delivery Control (IDC) */
#define IDC_BASE		0x4000
#define IDC_SIZE		0x20
#define  IDC_CLAIMI		0x1c

static void *imsic;

/*
 * When destroying cells, legacy IRQs need to be assigned back to the root
 * cell. Therefore, we need to keep track of their previous state: The target,
 * source configuration and enable state.
 */
static struct {
	u32 sourcecfg[MAX_IRQS];
	u32 target[MAX_IRQS];
	u32 enabled[MAX_IRQS / (sizeof(u32) * 8)];
} shadow;

static inline u32 aplic_read(u32 reg)
{
	return mmio_read32(irqchip.base + reg);
}

static inline void aplic_write(u32 reg, u32 value)
{
	mmio_write32(irqchip.base + reg, value);
}

static inline u32 aplic_read_setie(unsigned short no)
{
	return aplic_read(SETIE_BASE + no * IRQCHIP_REG_SZ);
}

static inline bool aplic_irq_is_enabled(unsigned int irq)
{
	u32 en = aplic_read_setie(irq / IRQCHIP_BITS_PER_REG);

	return !!(en & IRQ_MASK(irq));
}

static inline u32 aplic_read_target(unsigned int irq)
{
	return aplic_read(TARGET_BASE + (irq - 1) * IRQCHIP_REG_SZ);
}

static inline void aplic_write_target(unsigned int irq, u32 val)
{
	aplic_write(TARGET_BASE + (irq - 1) * IRQCHIP_REG_SZ, val);
}

static inline u32 aplic_read_sourcecfg(unsigned int irq)
{
	return aplic_read(SOURCECFG_BASE + (irq - 1) * IRQCHIP_REG_SZ);
}

static inline void aplic_write_sourcecfg(unsigned int irq, u32 val)
{
	aplic_write(SOURCECFG_BASE + (irq - 1) * IRQCHIP_REG_SZ, val);
}

static inline void aplic_write_clrienum(unsigned int irq)
{
	aplic_write(CLRIENUM, irq);
}

static void aplic_adjust_irq_target(struct cell *cell, unsigned int irq)
{
	u32 target, hart_index;
	unsigned int cpu;

	if (cell == &root_cell) {
		aplic_write_sourcecfg(irq, shadow.sourcecfg[irq]);
		aplic_write_target(irq, shadow.target[irq]);
		if (irq_bitmap_test(shadow.enabled, irq))
			aplic_write(SETIENUM, irq);
		else
			aplic_write(CLRIENUM, irq);
		return;
	}

	target = aplic_read_target(irq);
	hart_index =  target >> TARGET_HART_SHIFT;
	for_each_cpu(cpu, &cell->cpu_set)
		if (public_per_cpu(cpu)->phys_id == hart_index)
			return;

	/*
	 * Disable the IRQ. As IRQs must globally stay enabled in DOMAINCFG, we
	 * need to selecively disable it here to prevent spurious IRQs in the
	 * new cell.
	 */
	aplic_write_sourcecfg(irq, 0);

	/* We have to adjust the IRQ. Locate it on the first CPU of the cell */
	hart_index = public_per_cpu(first_cpu(&cell->cpu_set))->phys_id;

	target = (hart_index << TARGET_HART_SHIFT);
	if (imsic)
		target |= (cell->arch.vs_file << TARGET_GUEST_SHIFT) |
			  (target & TARGET_EIID_MSK);
	else
		target |= (target & TARGET_PRIO_MSK); /* preserve priority */

	aplic_write_target(irq, target);
}

static inline u32 aplic_read_idc(unsigned long hart, unsigned long reg)
{
	return aplic_read(IDC_BASE + IDC_SIZE * hart + reg);
}

static inline u32 aplic_read_claimi(unsigned long hart)
{
	return aplic_read_idc(hart, IDC_CLAIMI);
}

static int aplic_claim_irq(void)
{
	unsigned short source;
	unsigned int hart;
	u32 claimi;

	if (imsic)
		return -EINVAL;

	hart = phys_processor_id();

	claimi = aplic_read_claimi(hart);

	source = (claimi >> 16) & 0x7ff;
	if (source == 0) /* spurious IRQ, should not happen */
		return -EINVAL;

	if (source > irqchip_max_irq())
		return -EINVAL;

	irqchip.pending[hart] = claimi;

	return 0;
}

static inline void aplic_passthru(const struct mmio_access *access)
{
	aplic_write(access->address, access->value);
}

static inline unsigned int idc_to_hart(unsigned int idc)
{
	/*
	 * For the moment, assume that we only have one APLIC and #IDC = #HART
	 */
	return idc;
}

static inline enum mmio_result
aplic_handle_claimi(struct mmio_access *access, unsigned int idc)
{
	unsigned long hart = idc_to_hart(idc);

	if (access->is_write)
		return MMIO_ERROR;

	access->value = irqchip.pending[hart];

	irqchip.pending[hart] = aplic_read(access->address);

	guest_clear_ext();
	ext_enable();

	return MMIO_HANDLED;
}

static inline enum mmio_result aplic_handle_idc(struct mmio_access *access)
{
	unsigned long hart;
	unsigned int cpu, idc, reg;

	if (imsic)
		return MMIO_ERROR;

	idc = (access->address - IDC_BASE) / IDC_SIZE;
	reg = access->address % IDC_SIZE;

	/*
	 * It is clear that a hart is allowed to access its own IDC.
	 * But we also need to allow accesses to IDCs of neighboured
	 * harts within the cell.
	 *
	 * In (probably) 99% of all cases, the current active CPU will access
	 * its own context. So do this simple check first, and check other
	 * contexts of the cell (for loop) later. This results in a bit more
	 * complex code, but results in better performance.
	 */
	hart = phys_processor_id();
	if (idc_to_hart(idc) == hart)
		goto allowed;

	/*
	 * If we land here, then a HART accesses the IDC register of a
	 * neighboured HART. In this case, we forbid the access if a HART tries
	 * to cross-claim the interrupt. The reason is simple: If we would
	 * allow cross-core claiming of IRQs, then we would need to grab a lock
	 * inside aplic_handle_claimi() to prevent race conditions, which is
	 * too costful. I never saw any cross-HART irq claims.
	 */
	 if (reg == IDC_CLAIMI)
		return trace_error(MMIO_ERROR);

	for_each_cpu_except(cpu, &this_cell()->cpu_set, this_cpu_id())
		if (idc_to_hart(public_per_cpu(cpu)->phys_id) == idc)
			goto passthru;

	return trace_error(MMIO_ERROR);

allowed:
	if (reg == IDC_CLAIMI)
		return aplic_handle_claimi(access, hart);

passthru:
	aplic_passthru(access);

	return MMIO_HANDLED;
}

static inline enum mmio_result aplic_handle_ienum(struct mmio_access *access)
{
	struct cell *cell = this_cell();
	unsigned int target;

	/* Spec: A read always returns zero */
	if (!access->is_write) {
		access->value = 0;
		return MMIO_HANDLED;
	}

	/* Here we are in the write case */
	if (irqchip_virq_in_cell(cell, access->value)) {
		target = access->value - cell->config->vpci_irq_base;
		if (access->address == SETIENUM)
			cell->arch.aplic_virq.enabled |= (1 << target);
		else
			cell->arch.aplic_virq.enabled &= ~(1 << target);

		return MMIO_HANDLED;
	}

	if (!irqchip_irq_in_cell(cell, access->value))
		return MMIO_ERROR;

	if (access->address == SETIENUM)
		irq_bitmap_set(shadow.enabled, access->value);
	else
		irq_bitmap_clear(shadow.enabled, access->value);

	aplic_passthru(access);

	return MMIO_HANDLED;
}

static inline enum mmio_result
aplic_handle_sourcecfg(struct mmio_access *access)
{
	unsigned int irq;

	/* Check if the source IRQ belongs to the cell */
	irq = (access->address - SOURCECFG_BASE) / IRQCHIP_REG_SZ + 1;

	if (irqchip_virq_in_cell(this_cell(), irq)) {
		/*
		 * Actually, we don't need sourcecfg for vIRQs at all. Just
		 * emulate some 'sane' behaviour.
		 */
		if (!access->is_write)
			access->value = 6;
		else if (access->value == 6 || !access->value)
			return MMIO_HANDLED;

		return MMIO_ERROR;
	}

	/* If not, simply ignore the access. */
	if (!irqchip_irq_in_cell(this_cell(), irq)) {
		if (!access->is_write)
			access->value = 0;
		return MMIO_HANDLED;
	}

	/* If read, then pass through */
	if (!access->is_write) {
		access->value = aplic_read(access->address);
		return MMIO_HANDLED;
	}

	/* Don't support delegations at the moment */
	if (access->value & (1 << 10)) /* Delegation */
		return MMIO_ERROR;

	if (this_cell() == &root_cell)
		shadow.sourcecfg[irq] = access->value;

	/* If no delegations, simply pass through */
	aplic_passthru(access);

	return MMIO_HANDLED;
}

static bool hart_in_cell(struct cell *cell, unsigned long hart)
{
	unsigned int cpu;

	for_each_cpu(cpu, &cell->cpu_set)
		if (public_per_cpu(cpu)->phys_id == hart)
			return true;

	return false;
}

static inline enum mmio_result
aplic_handle_virq_target(struct mmio_access *access, unsigned int irq)
{
	struct cell *cell = this_cell();
	unsigned int cpu, *virq_target;
	unsigned long hart;
	u32 target;

	irq -= cell->config->vpci_irq_base;
	virq_target = &cell->arch.aplic_virq.target[irq];

	if (!access->is_write) {
		access->value = public_per_cpu(*virq_target)->phys_id <<
				TARGET_HART_SHIFT;
		if (imsic)
			access->value |= cell->arch.aplic_virq.eiid[irq];
		return MMIO_HANDLED;
	}

	target = access->value >> TARGET_HART_SHIFT;

	for_each_cpu(cpu, &cell->cpu_set) {
		hart = public_per_cpu(cpu)->phys_id;
		if (hart == target)
			goto write_allowed;
	}

	return MMIO_ERROR;

write_allowed:
	*virq_target = cpu;
	if (imsic) {
		cell->arch.aplic_virq.eiid[irq] =
			access->value & TARGET_EIID_MSK;
	}

	return MMIO_HANDLED;
}

static inline enum mmio_result aplic_handle_target(struct mmio_access *access)
{
	struct cell *cell = this_cell();
	unsigned int irq;
	u32 target, value;

	/* Check if the source IRQ belongs to the cell */
	irq = (access->address - TARGET_BASE) / IRQCHIP_REG_SZ + 1;

	if (irqchip_virq_in_cell(cell, irq))
		return aplic_handle_virq_target(access, irq);

	/* If not, simply ignore the access. */
	if (!irqchip_irq_in_cell(cell, irq)) {
		if (!access->is_write)
			access->value = 0;
		return MMIO_HANDLED;
	}

	/* If read, then pass through, but take care on the guest index */
	if (!access->is_write) {
		access->value = aplic_read(access->address);
		if (imsic)
			access->value &= ~TARGET_GUEST_MSK;

		return MMIO_HANDLED;
	}

	/* Here we are in the write case */
	target = access->value >> TARGET_HART_SHIFT;

	/*
	 * Linux initialises all targets with default priorities, even if the
	 * IRQ doesn't belong to it. Just return success.
	 */
	if (!hart_in_cell(cell, target))
		return MMIO_HANDLED;

	value = access->value;
	if (imsic) {
		value &= ~TARGET_GUEST_MSK;
		value |= cell->arch.vs_file << TARGET_GUEST_SHIFT;
	}
	aplic_write(access->address, value);

	/* Store the shadow, if we're on the root cell */
	if (cell == &root_cell)
		shadow.target[irq] = value;

	return MMIO_HANDLED;
}

static inline enum mmio_result aplic_handle_clrie(struct mmio_access *access)
{
	unsigned int idx;
	u32 allowed;

	idx = (access->address - CLRIE_START) / IRQCHIP_REG_SZ;

	/* Only allow clearing of IRQs that are in the cell */
	allowed = this_cell()->arch.irq_bitmap[idx] & access->value;

	if (this_cell() == &root_cell)
		shadow.enabled[idx] &= ~allowed;

	aplic_write(access->address, allowed);

	return MMIO_HANDLED;
}

static inline enum mmio_result
aplic_handle_domaincfg(struct mmio_access *access)
{
	u32 restricted_mask;

	/*
	 * Domaincfg is handled as follows: We can not allow guests to globally
	 * disable IRQs (i.e., modify the IE bit of the APLIC) as this would
	 * affect other cells. The easy strategy for the moment is, to simple
	 * ignore disabling of the IE bit. This avoids complex softemulation,
	 * and works for most situations, as IRQs that are assigned to a cell
	 * are disabled up on cell creation.
	 *
	 * So if Domaincfg is read, deliver that IE is enabled, and that's it.
	 * Ignore any modification of the IE bit, and don't allow to touch
	 * other bits, such as DM and BE.
	 */
	 if (!access->is_write) {
		access->value = DOMAINCFG_DEFAULT;
		if (imsic)
			access->value |= (1 << DOMAINCFG_DM);
		return MMIO_HANDLED;
	}

	restricted_mask = (1 << DOMAINCFG_BE);
	if (!imsic)
		restricted_mask |= (1 << DOMAINCFG_DM);

	if (access->value & restricted_mask)
		return MMIO_ERROR;

	return MMIO_HANDLED;
}

static inline enum mmio_result aplic_in_clrip(struct mmio_access *access)
{
	unsigned int no;
	no = (access->address - IN_CLRIP_START) / IRQCHIP_REG_SZ;
	u32 bitmap;

	/* Not implemented as never seen */
	if (!imsic)
		return MMIO_ERROR;

	/* Not implemented as never seen */
	if (access->is_write)
		return MMIO_ERROR;

	/* Read case */
	bitmap = this_cell()->arch.irq_bitmap[no];
	access->value = aplic_read(access->address);
	access->value = access->value & bitmap;

	return MMIO_HANDLED;
}

static inline enum mmio_result aplic_setipnum(struct mmio_access *access)
{
	/* Not implemented as never seen */
	if (!imsic)
		return MMIO_ERROR;

	if (!access->is_write) {
		access->value = 0;
		return MMIO_HANDLED;
	}

	if (irqchip_irq_in_cell(this_cell(), access->value)) {
		aplic_passthru(access);
		return MMIO_HANDLED;
	}

	return MMIO_ERROR;
}

static enum mmio_result aplic_handler(void *arg, struct mmio_access *access)
{
	enum mmio_result res = MMIO_ERROR;

	switch (access->address) {
	case REGISTER(DOMAINCFG):
		res = aplic_handle_domaincfg(access);
		break;

	case REG_RANGE(SOURCECFG_BASE, SOURCECFG_END):
		res = aplic_handle_sourcecfg(access);
		break;

	case REGISTER(SETIENUM):
	case REGISTER(CLRIENUM):
		res = aplic_handle_ienum(access);
		break;

	case REGISTER(SETIPNUM_LE):
		res = aplic_setipnum(access);
		break;

	case REG_RANGE(IN_CLRIP_START, IN_CLRIP_END):
		res = aplic_in_clrip(access);
		break;

	case REG_RANGE(CLRIE_START, CLRIE_END):
		res = aplic_handle_clrie(access);
		break;

	case REG_RANGE(TARGET_BASE, IDC_BASE):
		res = aplic_handle_target(access);
		break;

	case REG_RANGE(IDC_BASE, IDC_BASE + IDC_SIZE * MAX_CPUS):
		res = aplic_handle_idc(access);
		break;

	default:
		printk("Unknown APLIC access: 0x%lx - 0x%lx - %s\n",
		       access->address, access->value,
		       access->is_write ? "Write" : "Read");
		break;
	}

	return res;
}

static void migrate_irq(unsigned int irq, u32 file)
{
	u32 value;

	value = aplic_read_target(irq) & ~TARGET_GUEST_MSK;
	value |= file << TARGET_GUEST_SHIFT;
	aplic_write_target(irq, value);
}

static int aplic_init(void)
{
	unsigned short irq, vs_file;
	u32 sourcecfg;
	bool enabled;

	vs_file = this_cell()->arch.vs_file;
	if (aplic_read(DOMAINCFG) & (1 << DOMAINCFG_DM)) {
		if (!vs_file) {
			printk("FATAL: MSI Delivery enabled, "
			       "but no VS-file specified.\n");
			return -EINVAL;
		}
		imsic = paging_map_device(imsic_base(), imsic_size());
		if (!imsic)
			return -ENOMEM;

	} else if (vs_file) {
		printk("FATAL: VS-file specified, while MSI "
		       "delivery mode not enabled\n");
		return -EINVAL;
	}

	/*
	 * If we check during early initialisation if all enabled IRQs belong
	 * to the root cell, then we don't need to check if an IRQ belongs to a
	 * cell on arrival.
	 */
	for (irq = 1; irq < MAX_IRQS; irq++) {
		sourcecfg = aplic_read_sourcecfg(irq);
		if (sourcecfg & (1 << SOURCECFG_D)) {
			printk("IRQ delegation not supported: %d\n", irq);
			return -ENOSYS;
		}

		enabled = aplic_irq_is_enabled(irq);
		if (irqchip_irq_in_cell(&root_cell, irq)) {
			if (imsic)
				migrate_irq(irq, vs_file);

			shadow.sourcecfg[irq] = sourcecfg;
			shadow.target[irq] = aplic_read_target(irq);
			if (enabled)
				irq_bitmap_set(shadow.enabled, irq);
		} else if (enabled) {
			printk("Error: IRQ %u active in root cell\n",
			       irq);
			return trace_error(-EINVAL);
		}
	}

	return 0;
}

static void aplic_register_virq(struct cell *cell, unsigned int irq)
{
	unsigned int *virq_target;
	unsigned int index;

	index = irq - cell->config->vpci_irq_base;
	if (index >= APLIC_MAX_VIRQ) {
		printk("FATAL: aplic: too much vIRQs\n");
		panic_stop();
	}

	spin_lock(&cell->arch.virq_lock);
	virq_target = &cell->arch.aplic_virq.target[index];
	if (!cell_owns_cpu(cell, *virq_target))
		*virq_target = first_cpu(&cell->cpu_set);

	irq_bitmap_set(cell->arch.virq_present_bitmap, irq);
	spin_unlock(&cell->arch.virq_lock);
}

static void aplic_unregister_virq(struct cell *cell, unsigned int irq)
{
	unsigned int index, cpu;

	index = irq - cell->config->vpci_irq_base;

	spin_lock(&cell->arch.virq_lock);
	cell->arch.aplic_virq.enabled &= ~(1 << index);

	for_each_cpu(cpu, &cell->cpu_set)
		public_per_cpu(cpu)->virq.aplic_pending &= ~(1 << index);

	irq_bitmap_clear(cell->arch.virq_present_bitmap, irq);
	spin_unlock(&cell->arch.virq_lock);
}

static void
imsic_inject_irq(unsigned long hart, unsigned int file, unsigned int eiid)
{
	void *target;

	target = imsic + hart * imsic_stride_size() + file * 0x1000;
	mmio_write32(target, eiid);
}

static void aplic_send_virq(struct cell *cell, unsigned int irq)
{
	unsigned int index;
	unsigned int target_cpu;
	struct public_per_cpu *pcpu;

	spin_lock(&cell->arch.virq_lock);
	if (!irqchip_virq_in_cell(cell, irq)) {
		printk("vIRQ not present in destination\n");
		goto out;
	}

	index = irq - cell->config->vpci_irq_base;
	target_cpu = cell->arch.aplic_virq.target[index];
	pcpu = public_per_cpu(target_cpu);

	if (imsic) {
		imsic_inject_irq(target_cpu, cell->arch.vs_file,
				 cell->arch.aplic_virq.eiid[index]);
		goto out;
	}

	pcpu->virq.aplic_pending |= (1 << index);

	memory_barrier();
	arch_send_event(pcpu);

out:
	spin_unlock(&cell->arch.virq_lock);
}

/* We must arrive with virq_lock being held */
static bool aplic_inject_pending_virqs(void)
{
	struct cell *cell = this_cell();
	unsigned int *pending;
	unsigned int virq;

	if (imsic)
		return false;

	pending = &this_cpu_public()->virq.aplic_pending;
	if (!*pending)
		return false;

	virq = ffsl(*pending);
	*pending &= ~(1 << virq);

	virq += cell->config->vpci_irq_base;
	irqchip.pending[this_cpu_public()->phys_id] = virq << 16;

	return true;
}

static void aplic_shutdown(void)
{
	unsigned short irq;

	if (!imsic)
		return;

	/* Migrate VS-Mode IRQs back to S-Mode */
	for (irq = 1; irq < irqchip_max_irq(); irq++)
		if (irqchip_irq_in_cell(&root_cell, irq))
			migrate_irq(irq, 0);

	paging_unmap_device(imsic_base(), imsic, imsic_size());
}

const struct irqchip irqchip_aplic = {
	.init = aplic_init,
	.shutdown = aplic_shutdown,
	.claim_irq = aplic_claim_irq,
	.adjust_irq_target = aplic_adjust_irq_target,
	.mmio_handler = aplic_handler,

	.register_virq = aplic_register_virq,
	.unregister_virq = aplic_unregister_virq,
	.send_virq = aplic_send_virq,
	.inject_pending_virqs = aplic_inject_pending_virqs,
};
