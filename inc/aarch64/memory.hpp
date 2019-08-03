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

#define PAGE_BITS       12
#define PAGE_SIZE       BITN (PAGE_BITS)
#define OFFS_MASK       (PAGE_SIZE - 1)

// Global Area                [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_GLB_GICD   0x0000ffffffff0000      // 511 511 511 496  64K
#define MMAP_GLB_UART   0x0000ffffffe00000      // 511 511 511 000   4K
#define MMAP_GLB_DATA   0x0000ffffc0000000      // 511 511 000 000 256M
#define LINK_ADDR       0x0000ff8000000000      // 511 000 000 000

// CPU-Local Area             [--PTE--]---      // ^39 ^30 ^21 ^12
#define MMAP_CPU_DATA   0x0000ff7ffffff000      // 510 511 511 511   4K
#define MMAP_CPU_DSTK   0x0000ff7fffffd000      // 510 511 511 509   4K + gap
#define MMAP_CPU_MAP1   0x0000ff7ffc800000      // 510 511 484 000   4M + gap
#define MMAP_CPU_MAP0   0x0000ff7ffc000000      // 510 511 480 000   4M + gap

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
