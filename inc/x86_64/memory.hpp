/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "alignment.hpp"

#define LOAD_ADDR       ALIGNMENT_ADDR (ALIGNMENT_NOVA)

#define PTE_BPL         9
#define PAGE_BITS       12
#define LEVL_BITS(L)    ((L) * PTE_BPL + PAGE_BITS)
#define PAGE_SIZE(L)    BITN (LEVL_BITS (L))
#define OFFS_MASK(L)    (PAGE_SIZE (L) - 1)

#define VIRT_ADDR(L3,L2,L1,L0)  (VALN_SHIFT (0xffff, LEVL_BITS (4)) | VALN_SHIFT (L3, LEVL_BITS (3)) | VALN_SHIFT (L2, LEVL_BITS (2)) | VALN_SHIFT (L1, LEVL_BITS (1)) | VALN_SHIFT (L0, LEVL_BITS (0)))

// Space-Local Area
#define MMAP_SPC_PIO_E  VIRT_ADDR (511, 511,   0,   2)
#define MMAP_SPC_PIO    VIRT_ADDR (511, 511,   0,   0)
#define MMAP_SPC        VIRT_ADDR (511, 511,   0,   0)

// CPU-Local Area
#define MMAP_CPU_DATA   VIRT_ADDR (511, 510, 511, 511)          //   4K
#define MMAP_CPU_DSTT   VIRT_ADDR (511, 510, 511, 391)          // Data Stack Top
#define MMAP_CPU_DSTB   VIRT_ADDR (511, 510, 511, 390)          // Data Stack Base
#define MMAP_CPU_ISTT   VIRT_ADDR (511, 510, 511, 389)          // Intr Stack Top
#define MMAP_CPU_ISTB   VIRT_ADDR (511, 510, 511, 388)          // Intr Stack Base
#define MMAP_CPU_DTKN  (VIRT_ADDR (511, 510, 511, 387) - 8)     // Data Supervisor Shadow Stack Token
#define MMAP_CPU_DSSS   VIRT_ADDR (511, 510, 511, 386)          // Data Supervisor Shadow Stack
#define MMAP_CPU_ITKN  (VIRT_ADDR (511, 510, 511, 385) - 8)     // Intr Supervisor Shadow Stack Token
#define MMAP_CPU_ISSS   VIRT_ADDR (511, 510, 511, 384)          // Intr Supervisor Shadow Stack
#define MMAP_CPU_APIC   VIRT_ADDR (511, 510, 511, 256)          //   4K
#define MMAP_CPU        VIRT_ADDR (511, 510, 511,   0)          //   2M

// Global Area
#define MMAP_GLB_TPM2   VIRT_ADDR (511, 510, 508, 320)          //   2M (0xfed40000 offset in 2M window)
#define MMAP_GLB_TXTC   VIRT_ADDR (511, 510, 508, 288)          //   2M (0xfed20000 offset in 2M window)
#define MMAP_GLB_TXTH   VIRT_ADDR (511, 510, 504,   0)          //   4M + gap
#define MMAP_GLB_MAP1   VIRT_ADDR (511, 510, 500,   0)          //   4M + gap
#define MMAP_GLB_MAP0   VIRT_ADDR (511, 510, 496,   0)          //   4M + gap
#define MMAP_GLB_UART   VIRT_ADDR (511, 510, 488,   0)          //  16M
#define MMAP_GLB_MMIO   VIRT_ADDR (511, 510, 448,   0)          //  64M
#define LINK_ADDR       VIRT_ADDR (511, 510,   0,   0)          // 896M

#define MMAP_GLB_CPUS   VIRT_ADDR (511, 509,   0,   0)          //   1G (262144 CPUs)
#define MMAP_GLB_PCIE   MMAP_GLB_CPUS
#define MMAP_GLB_PCIS   VIRT_ADDR (511, 253,   0,   0)          // 256G (1024 PCI Segment Groups)
#define MMAP_TMP_RW1E   MMAP_GLB_PCIS
#define MMAP_TMP_RW1S   VIRT_ADDR (511, 252,   0,   0)          //   1G (Remap Window 1)
#define MMAP_TMP_RW0E   MMAP_TMP_RW1S
#define MMAP_TMP_RW0S   VIRT_ADDR (511, 251,   0,   0)          //   1G (Remap Window 0)
#define BASE_ADDR       MMAP_GLB_PCIS

#define OFFSET          (LINK_ADDR - LOAD_ADDR)
