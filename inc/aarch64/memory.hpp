/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
#define LEVL_BITS(L)    ((L) * PTE_BPL + PAGE_BITS)
#define PAGE_SIZE(L)    BITN (LEVL_BITS (L))
#define OFFS_MASK(L)    (PAGE_SIZE (L) - 1)

#define VIRT_ADDR(L3,L2,L1,L0)  (VALN_SHIFT (L3, LEVL_BITS (3)) | VALN_SHIFT (L2, LEVL_BITS (2)) | VALN_SHIFT (L1, LEVL_BITS (1)) | VALN_SHIFT (L0, LEVL_BITS (0)))

// Global Area
#define MMAP_GLB_CPUS   VIRT_ADDR (511, 509,   0,   0)  //   1G (262144 CPUs)
#define MMAP_GLB_PCIE   MMAP_GLB_CPUS
#define MMAP_GLB_PCIS   VIRT_ADDR (511, 253,   0,   0)  // 256G (1024 PCI Segment Groups)
#define MMAP_TMP_RW1E   MMAP_GLB_PCIS
#define MMAP_TMP_RW1S   VIRT_ADDR (511, 252,   0,   0)  //   1G (Remap Window 1)
#define MMAP_TMP_RW0E   MMAP_TMP_RW1S
#define MMAP_TMP_RW0S   VIRT_ADDR (511, 251,   0,   0)  //   1G (Remap Window 0)

#define MMAP_GLB_MAP1   VIRT_ADDR (511,   0, 500,   0)  //   4M + gap
#define MMAP_GLB_MAP0   VIRT_ADDR (511,   0, 496,   0)  //   4M + gap
#define MMAP_GLB_GICD   VIRT_ADDR (511,   0, 488,  32)  //  64K
#define MMAP_GLB_GICC   VIRT_ADDR (511,   0, 488,  16)  //  64K
#define MMAP_GLB_GICH   VIRT_ADDR (511,   0, 488,   0)  //  64K
#define MMAP_GLB_UART   VIRT_ADDR (511,   0, 480,   0)  //  16M
#define MMAP_GLB_SMMU   VIRT_ADDR (511,   0, 448,   0)  //  64M
#define LINK_ADDR       VIRT_ADDR (511,   0,   0,   0)  // 896M

// CPU-Local Area
#define MMAP_CPU_DATA   VIRT_ADDR (510, 511, 511, 511)  //   4K
#define MMAP_CPU_DSTT   VIRT_ADDR (510, 511, 511, 510)  // Data Stack Top
#define MMAP_CPU_DSTB   VIRT_ADDR (510, 511, 511, 509)  // Data Stack Base
#define MMAP_CPU_GICR   VIRT_ADDR (510, 511, 511,   0)  // 256K

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
