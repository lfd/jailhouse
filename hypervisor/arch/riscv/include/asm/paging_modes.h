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

extern const struct paging riscv_Sv39[];
extern const struct paging riscv_Sv48[];
extern const struct paging riscv_Sv39x4[];
extern const struct paging riscv_Sv48x4[];
