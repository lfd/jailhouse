/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for S32G board
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#define MIB			(1024ULL * 1024ULL)
#define GIB			(MIB * 1024ULL)

#define RESERVATION_BASE	0x880000000ULL
#define RESERVATION_SIZE	(64 * MIB)
#define RESERVATION_END		(RESERVATION_BASE + RESERVATION_SIZE)

#define JH_BASE			(RESERVATION_BASE + RESERVATION_SIZE - JH_SIZE)
#define JH_SIZE			(6 * MIB)

#define SHMEM_NET_0_SIZE	(1 * MIB)
#define SHMEM_NET_0_BASE	(JH_BASE - SHMEM_NET_0_SIZE)

#define INMATES_SIZE		(RESERVATION_SIZE - JH_SIZE - SHMEM_NET_0_SIZE)
#define INMATES_BASE		RESERVATION_BASE

#define RAM_UPPER_SIZE		(2 * GIB)
