/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013-2015
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_DRIVER_MAIN_H
#define _JAILHOUSE_DRIVER_MAIN_H

#include <linux/mutex.h>

#include "cell.h"

extern struct mutex jailhouse_lock;
extern bool jailhouse_enabled;
extern struct jailhouse_console *console_page;
extern bool console_available;

void *jailhouse_ioremap(phys_addr_t phys, unsigned long virt,
			unsigned long size);

#endif /* !_JAILHOUSE_DRIVER_MAIN_H */
