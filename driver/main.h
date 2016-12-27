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
#include <jailhouse/header.h>

#include "cell.h"

extern struct mutex jailhouse_lock;
extern bool jailhouse_enabled;

void *jailhouse_ioremap(phys_addr_t phys, unsigned long virt,
			unsigned long size);
int jailhouse_console_page_delta(char *dst, unsigned int head,
				 unsigned int *miss);

#endif /* !_JAILHOUSE_DRIVER_MAIN_H */
