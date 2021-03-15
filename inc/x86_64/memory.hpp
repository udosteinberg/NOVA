/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#define PTE_BPL         9
#define PAGE_BITS       12
#define PAGE_SIZE(L)    BITN ((L) * PTE_BPL + PAGE_BITS)
#define OFFS_MASK(L)    (PAGE_SIZE (L) - 1)

// Space-Local Area           [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_SPC_PIO_E  0xffffffffc0002000      // 511 511 000 002
#define MMAP_SPC_PIO    0xffffffffc0000000      // 511 511 000 000
#define MMAP_SPC        0xffffffffc0000000      // 511 511 000 000

// CPU-Local Area             [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_CPU_DATA   0xffffffffbffff000      // 511 510 511 511    4K
#define MMAP_CPU_DSTK   0xffffffffbfffd000      // 511 510 511 509    4K + gap
#define MMAP_CPU_APIC   0xffffffffbfffb000      // 511 510 511 507    4K + gap
#define MMAP_CPU        0xffffffffbfe00000      // 511 510 511 000    2M

// Global Area                [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_GLB_MAP1   0xffffffffbe800000      // 511 510 500 000    4M + gap
#define MMAP_GLB_MAP0   0xffffffffbe000000      // 511 510 496 000    4M + gap
#define MMAP_GLB_APIC   0xffffffffbd000000      // 511 510 488 000   16M
#define MMAP_GLB_UART   0xffffffffbc000000      // 511 510 480 000   16M
#define MMAP_GLB_SMMU   0xffffffffb8000000      // 511 510 448 000   64M
#define MMAP_GLB_CPUS   0xffffffffb0000000      // 511 510 384 000  128M
#define MMAP_GLB_PCIE   0xffffffffb0000000      // 511 510 384 000
#define MMAP_GLB_PCIS   0xffffffffa0000000      // 511 510 256 000  256M
#define LINK_ADDR       0xffffffff80000000      // 511 510 000 000  512M

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
