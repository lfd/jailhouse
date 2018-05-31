static inline void arch_disable_irqs(void)
{
	asm volatile("msr daifset, #3"); /* disable IRQs and FIQs */
}
