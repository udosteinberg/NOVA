/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#include "macros.hpp"

#define LOAD_ADDR       0x400000

#define PAGE_BITS       12
#define PAGE_SIZE       BITN (PAGE_BITS)
#define OFFS_MASK       (PAGE_SIZE - 1)

// Space-Local Area           [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_SPC_IOP_E  0xffffffffc0002000      // 511 511 000 002
#define MMAP_SPC_IOP    0xffffffffc0000000      // 511 511 000 000
#define MMAP_SPC        0xffffffffc0000000      // 511 511 000 000

// CPU-Local Area             [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_CPU_DATA   0xffffffffbffff000      // 511 510 511 511    4K
#define MMAP_CPU_SSTK   0xffffffffbfffe000      // 511 510 511 510    4K
#define MMAP_CPU_DSTK   0xffffffffbfffd000      // 511 510 511 509    4K
#define MMAP_CPU_APIC   0xffffffffbfffb000      // 511 510 511 507    4K + gap
#define MMAP_CPU_TMAP   0xffffffffbfa00000      // 511 510 509 000    4M
#define MMAP_CPU        0xffffffffb0000000      // 511 510 384 000

// Global Area                [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_GLB_PCIE   0xffffffffac000000      // 511 510 352 000   64M
#define MMAP_GLB_APIC   0xffffffffa8000000      // 511 510 320 000   64M
#define MMAP_GLB_UART   0xffffffffa4000000      // 511 510 288 000   64M
#define MMAP_GLB_SMMU   0xffffffffa0000000      // 511 510 256 000   64M
#define MMAP_GLB_DATA   0xffffffff90000000      // 511 510 128 000  256M
#define LINK_ADDR       0xffffffff80000000      // 511 510 000 000  256M

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
