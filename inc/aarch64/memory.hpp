/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#define PAGE_BITS       12
#define PAGE_SIZE       (1 << PAGE_BITS)
#define PAGE_MASK       (PAGE_SIZE - 1)

//                        [--PTE--]---      // ^39 ^30 ^21 ^12
#define DEV_GLOBL_UART  0xffffffe00000      // 511 511 511 000   4K
#define CPU_GLOBL_DATA  0xffffffc00000      // 511 511 510 000   2M
#define LINK_ADDR       0xff8000000000      // 511 000 000 000

#define CPU_LOCAL_DATA  0xff7ffffff000      // 510 511 511 511   4K
#define CPU_LOCAL_STCK  0xff7fffffd000      // 510 511 511 509   4K
#define CPU_LOCAL_TMAP  0xff7fc0000000      // 510 511 000 000   2M*2
#define CPU_LOCAL       0xff0000000000      // 510 000 000 000

#define USER_ADDR       (1ULL << IPA_BITS)
#define HIPB_ADDR       (USER_ADDR - PAGE_SIZE)
#define UTCB_ADDR       (HIPB_ADDR - PAGE_SIZE)

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
