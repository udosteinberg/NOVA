/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#define PAGE_BITS       12
#define PAGE_SIZE       BITN (PAGE_BITS)
#define OFFS_MASK       (PAGE_SIZE - 1)

// Global Area                [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_GLB_GICD   0x0000ffffffff0000      // 511 511 511 496  64K
#define MMAP_GLB_GICC   0x0000fffffffe0000      // 511 511 511 480  64K
#define MMAP_GLB_GICH   0x0000fffffffd0000      // 511 511 511 464  64K
#define MMAP_GLB_UART   0x0000ffffffe00000      // 511 511 511 000   4K
#define MMAP_GLB_SMMU   0x0000ffffd0000000      // 511 511 128 000 256M
#define MMAP_GLB_DATA   0x0000ffffc0000000      // 511 511 000 000 256M
#define LINK_ADDR       0x0000ff8000000000      // 511 000 000 000

// CPU-Local Area             [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_CPU_DATA   0x0000ff7ffffff000      // 510 511 511 511   4K
#define MMAP_CPU_STCK   0x0000ff7fffffd000      // 510 511 511 509   4K + gap
#define MMAP_CPU_GICR   0x0000ff7fffe00000      // 510 511 511 000 256K
#define MMAP_CPU_TMAP   0x0000ff7fffa00000      // 510 511 509 000   4M

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
