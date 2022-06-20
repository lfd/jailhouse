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

#include <jailhouse/paging.h>
#include <jailhouse/cell.h>
#include <jailhouse/string.h>
#include <jailhouse/unit.h>
#include <jailhouse/control.h>
#include <jailhouse/processor.h>
#include <jailhouse/printk.h>
#include <asm/csr64.h>

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

#define REG_SZ			4
#define REG_RANGE(A, B)		(A)...((B) - REG_SZ)

#define PLIC_BITS_PER_REG	(REG_SZ * 8)
#define IRQ_BIT(irq)		((irq) % PLIC_BITS_PER_REG)
#define IRQ_MASK(irq)		(1 << IRQ_BIT(irq))


/* Could also be used for arm-common/irqchip.c */
#define IRQCHIP_PINS \
	(sizeof(((struct jailhouse_irqchip *)0)->pin_bitmap) * 8)

#define IRQ_BITMAP_PINS \
	(sizeof(((struct cell *)0)->arch.irq_bitmap) * 8)

static void *plic_base;
static unsigned long pending[MAX_CPUS];

static inline unsigned long plic_phys(void)
{
	return system_config->platform_info.riscv.plic.base_address;
}

static inline s16 hart_to_context(unsigned int hartid)
{
	if (hartid > ARRAY_SIZE(
		system_config->platform_info.riscv.plic.hart_to_context))
		return -1;

	return system_config->platform_info.riscv.plic.hart_to_context[hartid];
}

static inline u16 plic_max_irq(void)
{
	return system_config->platform_info.riscv.plic.max_irq;
}

static inline u16 plic_max_priority(void)
{
	return system_config->platform_info.riscv.plic.max_priority;
}

static inline unsigned int plic_size(void)
{
	return system_config->platform_info.riscv.plic.size;
}

static inline void ext_enable(void)
{
	csr_set(sie, IE_EIE);
}

static inline void ext_disable(void)
{
	csr_clear(sie, IE_EIE);
}

static inline void guest_clear_ext(void)
{
	csr_clear(CSR_HVIP, (1 << IRQ_S_EXT) << VSIP_TO_HVIP_SHIFT);
}

static inline void guest_inject_ext(void)
{
	csr_set(CSR_HVIP, (1 << IRQ_S_EXT) << VSIP_TO_HVIP_SHIFT);
}

static inline bool guest_ext_pending(void)
{
	return !!(csr_read(CSR_HVIP) &
			((1 << IRQ_S_EXT) << VSIP_TO_HVIP_SHIFT));
}

static inline u32 plic_read(u32 reg)
{
	return mmio_read32(plic_base + reg);
}

static inline void plic_write(u32 reg, u32 value)
{
	mmio_write32(plic_base + reg, value);
}

static inline u32 plic_read_context(u32 context, u32 off)
{
	return plic_read(PLIC_CTX_BASE + context * PLIC_CTX_SZ + off);
}

static inline u32 plic_read_claim(u32 context)
{
	return plic_read_context(context, PLIC_CTX_CLAIM);
}

static inline u32 plic_en_reg(s16 context, unsigned int irq)
{
	u32 reg;

	reg = PLIC_ENABLE_BASE + (context * PLIC_ENABLE_OFF) +
	      (irq / PLIC_BITS_PER_REG) * REG_SZ;

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

static inline void plic_disable_irq(s16 context, unsigned int irq)
{
	u32 val;

	val = plic_read_enabled(context, irq) & ~IRQ_MASK(irq);
	plic_write_enabled(context, irq, val);
}

static bool irq_bitmap_test(u32 *bitmap, unsigned int irq)
{
	u32 val;

	if (irq >= plic_max_irq())
		return false;

	val = bitmap[irq / PLIC_BITS_PER_REG];

	return !!(val & IRQ_MASK(irq));
}

static inline void irq_bitmap_set(u32 *bitmap, unsigned int irq)
{
	bitmap[irq / PLIC_BITS_PER_REG] |= IRQ_MASK(irq);
}

static inline void irq_bitmap_clear(u32 *bitmap, unsigned int irq)
{
	bitmap[irq / PLIC_BITS_PER_REG] &= ~IRQ_MASK(irq);
}

inline bool irqchip_irq_in_cell(struct cell *cell, unsigned int irq)
{
	return irq_bitmap_test(cell->arch.irq_bitmap, irq);
}

int plic_set_pending(void)
{
	int my_context;
	u32 irq;
	unsigned int cpuid;

	this_cpu_public()->stats[JAILHOUSE_CPU_STAT_VMEXITS_VIRQ]++;

	cpuid = phys_processor_id();

	/* Assume that phys_processor_id always returns something < 64 */
	my_context = hart_to_context(cpuid);
	if (my_context < 0)
		return -ENOSYS;

	irq = plic_read_claim(my_context);
	if (irq == 0) /* spurious IRQ, should not happen */
		return -EINVAL;

	if (irq > plic_max_irq())
		return -EINVAL;

	pending[cpuid] = irq;
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

static inline void plic_passthru(const struct mmio_access *access)
{
	plic_write(access->address, access->value);
}

static inline enum mmio_result
plic_handle_context_claim(struct mmio_access *access, unsigned long hart)
{
	if (!access->is_write) {
		access->value = pending[hart];
		return MMIO_HANDLED;
	}

	/* claim write case */
	if (access->value != pending[hart]) {
		printk("FATAL: Guest acknowledged non-pending IRQ %lu\n",
		       access->value);
		return MMIO_ERROR;
	}

	plic_write(access->address, access->value);

	/* Check if there's another physical IRQ pending */
	/* TODO: This is where we would need to prioritise vIRQs */
	pending[hart] = plic_read(access->address);
	if (pending[hart])
		return MMIO_HANDLED;

	guest_clear_ext();
	ext_enable();

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

	irq = access->address / REG_SZ;

	if (!irqchip_irq_in_cell(this_cell(), irq))
		return MMIO_ERROR;

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
	struct public_per_cpu *pc;
	u32 irq_allowed_bitmap;
	unsigned int idx, cpu;
	short int ctx;

	ctx = (access->address - PLIC_ENABLE_BASE) / PLIC_ENABLE_OFF;

	/* Does the context even belong to one of the cell's CPUs? */
	for_each_cpu(cpu, &this_cell()->cpu_set) {
		pc = public_per_cpu(cpu);
		if (hart_to_context(pc->phys_id) == ctx)
			goto allowed;
	}

	/*
	 * FIXME: Why does Linux read non-allowed ctxs? This seems to be an
	 * actual bug in Linux. When we remove a CPU from Linux, and we later
	 * change the affinity of the IRQ, then Linux will try to access
	 * Contexts which it is not in charge of any longer. While Linux
	 * disables IRQs, it does not adjust smp_affinities when removing CPUs.
	 *
	 * For the moment, and as a workaround, simply report any read as 0,
	 * and forbid writes != 0.
	 *
	 * ... Okay, we really have a Linux bug here.
	 *  (a) Linux doesn't remove the affinity from removed CPUs
	 *  (b) Linux allows to set affinity to non-present CPUs
	 *
	 * Actually, we should always return MMIO_ERROR here.
	 */

#if 1
	if (!access->is_write) {
		access->value = 0;
	} else if (access->value != 0)
		return MMIO_ERROR;
	return MMIO_HANDLED;
#else
	return MMIO_ERROR;
#endif

allowed:
	/*
	 * Now we have to check if we have a read or write access. In case of
	 * reads, simply return the real value of the PLIC.
	 *
	 * In case of writes, compare against the irq_bitmap, if we're allowed
	 * to perform the write.
	 */
	idx = ((access->address - PLIC_ENABLE_BASE) % PLIC_ENABLE_OFF)
		* 8 / PLIC_BITS_PER_REG;

	if (!access->is_write) {
		access->value = plic_read(access->address);
		return MMIO_HANDLED;
	}

	/* write case */
	irq_allowed_bitmap = this_cell()->arch.irq_bitmap[idx];

	if (access->value & ~irq_allowed_bitmap) {
		printk("FATAL: Cell enabled non-assigned IRQ\n");
		return MMIO_ERROR;
	}

	plic_passthru(access);

	return MMIO_HANDLED;
}

static enum mmio_result plic_handler(void *arg, struct mmio_access *access)
{
	/* only allow 32bit access */
	if (access->size != REG_SZ)
		return MMIO_ERROR;

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

static int plic_cell_init(struct cell *cell)
{
	const struct jailhouse_irqchip *chip;
	unsigned int n, pos;

	mmio_region_register(cell, plic_phys(), plic_size(), plic_handler,
			     cell);

	for_each_irqchip(chip, cell->config, n) {
		/* Only support one single PLIC at the moment */
		if (chip->address !=
		    system_config->platform_info.riscv.plic.base_address)
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

static int plic_init(void)
{
	unsigned int cpu, irq;
	s16 context;

	plic_base = paging_map_device(plic_phys(), plic_size());
	if (!plic_base)
		return -ENOMEM;

	plic_cell_init(&root_cell);

	/*
	 * If we check during early initialisation if all enabled IRQs belong
	 * to the root cell, then we don't need to check if an IRQ belongs to a
	 * cell on arrival.
	 */
	for_each_cpu(cpu, &root_cell.cpu_set) {
		context = hart_to_context(cpu);
		for (irq = 0; irq < plic_max_irq(); irq++) {
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

static void plic_cell_exit(struct cell *cell)
{
	const struct jailhouse_irqchip *chip;
	unsigned int n, pos;

	mmio_region_unregister(cell, plic_phys());

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

static void plic_shutdown(void)
{
	if (plic_base)
		paging_unmap_device(plic_phys(), plic_base, plic_size());
}

static unsigned int plic_mmio_count_regions(struct cell *cell)
{
	return 1;
}

static void plic_config_commit(struct cell *cell)
{
	unsigned int irq, n;
	s16 ctx;

	if (!cell)
		return;

	if (cell == &root_cell)
		return;

	for (irq = 0; irq < plic_max_irq(); irq++) {
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

		/* Disable the IRQ for each defined PLIC context. */
		for (n = 0; n < MAX_CPUS; n++) {
			ctx = hart_to_context(n);
			if (ctx == -1)
				// or break? Are there non-contiguous harts?
				continue;

			plic_disable_irq(ctx, irq);
		}
	}
}

DEFINE_UNIT (plic, "RISC-V PLIC");
