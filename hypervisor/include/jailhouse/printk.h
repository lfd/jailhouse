/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/paging.h>
#include <jailhouse/types.h>

void __attribute__((format(printf, 1, 2))) printk(const char *fmt, ...);

void __attribute__((format(printf, 1, 2))) panic_printk(const char *fmt, ...);

struct console {
	unsigned int start;
	char content[PAGE_SIZE - sizeof(unsigned int)];
} __attribute__((aligned(PAGE_SIZE)));

extern struct console *console;

#ifdef CONFIG_TRACE_ERROR
#define trace_error(code) ({						  \
	printk("%s:%d: returning error %s\n", __FILE__, __LINE__, #code); \
	code;								  \
})
#else /* !CONFIG_TRACE_ERROR */
#define trace_error(code)	code
#endif /* !CONFIG_TRACE_ERROR */

void arch_dbg_write_init(void);
extern void (*arch_dbg_write)(const char *msg);
