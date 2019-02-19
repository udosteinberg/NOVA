/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "board.hpp"
#include "macros.hpp"

#ifndef RAM_SIZE
#define RAM_SIZE        0x80000000              // Assume 2GiB populated
#endif

#ifndef RAM_BASE
#define LOAD_ADDR       0
#else
#define LOAD_ADDR       (RAM_BASE + RAM_SIZE - 0x2000000)
#endif

#define PTE_BPL         9
#define PAGE_BITS       12
#define PAGE_SIZE(L)    BITN ((L) * PTE_BPL + PAGE_BITS)
#define OFFS_MASK(L)    (PAGE_SIZE (L) - 1)

// Global Area                [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_GLB_PCIE   0x0000ff8140000000      // 511 005 000 000
#define MMAP_GLB_PCIS   0x0000ff8040000000      // 511 001 000 000    4G
#define MMAP_GLB_MAP1   0x0000ff803e800000      // 511 000 500 000    4M + gap
#define MMAP_GLB_MAP0   0x0000ff803e000000      // 511 000 496 000    4M + gap
#define MMAP_GLB_GICD   0x0000ff803d020000      // 511 000 488 032   64K
#define MMAP_GLB_GICC   0x0000ff803d010000      // 511 000 488 016   64K
#define MMAP_GLB_GICH   0x0000ff803d000000      // 511 000 488 000   64K
#define MMAP_GLB_UART   0x0000ff803c000000      // 511 000 480 000   16M
#define MMAP_GLB_SMMU   0x0000ff8038000000      // 511 000 448 000   64M
#define MMAP_GLB_CPUS   0x0000ff8030000000      // 511 000 384 000  128M
#define LINK_ADDR       0x0000ff8000000000      // 511 000 000 000  512M

// CPU-Local Area             [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_CPU_DATA   0x0000ff7ffffff000      // 510 511 511 511    4K
#define MMAP_CPU_DSTK   0x0000ff7fffffd000      // 510 511 511 509    4K + gap
#define MMAP_CPU_GICR   0x0000ff7fffe00000      // 510 511 511 000  256K

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
