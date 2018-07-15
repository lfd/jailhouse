/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2014
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

#include <inmate.h>

#define NUM_IDT_DESC		64

#define X2APIC_EOI		0x80b
#define X2APIC_SPIV		0x80f

#define APIC_EOI_ACK		0

struct desc_table_reg {
	u16 limit;
	u64 base;
} __attribute__((packed));

static u32 idt[NUM_IDT_DESC * 4];
static int_handler_t int_handler[NUM_IDT_DESC];

extern u8 irq_entry[];

static inline void write_idtr(struct desc_table_reg *val)
{
	asm volatile("lidt %0" : : "m" (*val));
}

void int_init(void)
{
	struct desc_table_reg dtr;

	write_msr(X2APIC_SPIV, 0x1ff);

	dtr.limit = NUM_IDT_DESC * 16 - 1;
	dtr.base = (u64)&idt;
	write_idtr(&dtr);
}

static void __attribute__((used)) handle_interrupt(unsigned int vector)
{
	int_handler[vector]();
	write_msr(X2APIC_EOI, APIC_EOI_ACK);
}

int int_set_handler(unsigned int vector, int_handler_t handler)
{
	unsigned long entry = (unsigned long)irq_entry + vector * 16;

	if (vector >= NUM_IDT_DESC)
		return -1;

	int_handler[vector] = handler;

	idt[vector * 4] = (entry & 0xffff) | (INMATE_CS64 << 16);
	idt[vector * 4 + 1] = 0x8e00 | (entry & 0xffff0000);
	idt[vector * 4 + 2] = entry >> 32;

	return 0;
}

#ifdef __x86_64__
asm(
".macro irq_prologue vector\n\t"
	"push %rdi\n\t"
	"mov $vector,%rdi\n\t"
	"jmp irq_common\n"
".endm\n\t"

	".global irq_entry\n\t"
	".balign 16\n"
"irq_entry:\n"
"vector=0\n"
".rept 64\n"
	"irq_prologue vector\n\t"
	"vector=vector+1\n\t"
	".balign 16\n\t"
".endr\n"

"irq_common:\n\t"
	"push %rax\n\t"
	"push %rcx\n\t"
	"push %rdx\n\t"
	"push %rsi\n\t"
	"push %r8\n\t"
	"push %r9\n\t"
	"push %r10\n\t"
	"push %r11\n\t"

	"call handle_interrupt\n\t"

	"pop %r11\n\t"
	"pop %r10\n\t"
	"pop %r9\n\t"
	"pop %r8\n\t"
	"pop %rsi\n\t"
	"pop %rdx\n\t"
	"pop %rcx\n\t"
	"pop %rax\n\t"
	"pop %rdi\n\t"

	"iretq"
);
#else
#error implement me!
#endif

void int_send_ipi(unsigned int cpu_id, unsigned int vector)
{
	write_msr(X2APIC_ICR, ((u64)cpu_id << 32) | APIC_LVL_ASSERT | vector);
}
