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
#define PAGE_SIZE       BIT64 (PAGE_BITS)
#define PAGE_MASK       (PAGE_SIZE - 1)

//                        [--PTE--]---      // ^39 ^30 ^21 ^12
#define DEV_GLOBL_GICD  0xffffffff0000      // 511 511 511 496  64K
#define DEV_GLOBL_GICC  0xfffffffe0000      // 511 511 511 480  64K
#define DEV_GLOBL_GICH  0xfffffffd0000      // 511 511 511 464  64K
#define DEV_GLOBL_UART  0xffffffe00000      // 511 511 511 000   4K
#define DEV_GLOBL_SMMU  0xffffc0000000      // 511 511 000 000
#define CPU_GLOBL_DATA  0xffff80000000      // 511 510 000 000   1G
#define LINK_ADDR       0xff8000000000      // 511 000 000 000

#define CPU_LOCAL_DATA  0xff7ffffff000      // 510 511 511 511   4K
#define CPU_LOCAL_STCK  0xff7fffffd000      // 510 511 511 509   4K
#define DEV_LOCAL_GICR  0xff7fffe00000      // 510 511 511 000 256K
#define CPU_LOCAL_TMAP  0xff7fc0000000      // 510 511 000 000   2M*2
#define CPU_LOCAL       0xff0000000000      // 510 000 000 000

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
