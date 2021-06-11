/*
 * Linker Script
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "arch.hpp"
#include "memory.hpp"

OUTPUT_ARCH(BFD_ARCH)
OUTPUT_FORMAT(BFD_FORMAT)
ENTRY(__start_bsp)

PHDRS
{
    init PT_LOAD FLAGS (6);
    kern PT_LOAD FLAGS (6);
}

SECTIONS
{
    .init LOAD_ADDR :
    {
        *(.init)
    } : init

    . = ALIGN(4K) + 4K; PROVIDE_HIDDEN (STACK = .);

    PROVIDE_HIDDEN (LOAD_E = .);

    .text . + OFFSET : AT (ADDR (.text) - OFFSET)
    {
        *(.text.hot .text.hot.*)
        *(.text .text.* .gnu.linkonce.t.*)
    } : kern

    .rodata : AT (ADDR (.rodata) - OFFSET)
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } : kern

    .init_array : AT (ADDR (.init_array) - OFFSET)
    {
        /*
         * Portability note: Some compilers (e.g., on MacOS) still use
         * .ctors instead of .init_array, so we need to handle both.
         */
        PROVIDE_HIDDEN (CTORS_L = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.65534) SORT_BY_INIT_PRIORITY (.ctors.00001)))
        PROVIDE_HIDDEN (CTORS_C = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.65533) SORT_BY_INIT_PRIORITY (.ctors.00002)))
        PROVIDE_HIDDEN (CTORS_S = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.*) SORT_BY_INIT_PRIORITY (.ctors.*)))
        KEEP (*(.init_array .ctors))
        PROVIDE_HIDDEN (CTORS_E = .);
    } : kern

    .data : AT (ADDR (.data) - OFFSET)
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } : kern

    .bss : AT (ADDR (.bss) - OFFSET)
    {
        *(.bss .bss.* .gnu.linkonce.b.*)

        . = ALIGN(4K);
        _mempool_l = .; PROVIDE_HIDDEN (_mempool_p = _mempool_l - OFFSET);

        PROVIDE_HIDDEN (PAGE_0 = .); PROVIDE_HIDDEN (FRAME_0 = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (PAGE_1 = .); PROVIDE_HIDDEN (FRAME_1 = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (PAGE_H = .); PROVIDE_HIDDEN (FRAME_H = . - OFFSET); . += 4K;

        PROVIDE_HIDDEN (PDBR  = . - OFFSET);
        PROVIDE_HIDDEN (LVL4  = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (LVL3L = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (LVL3H = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (LVL2L = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (LVL2H = . - OFFSET); . += 4K;

        _mempool_f = .;

        . += 16M;
        . = ALIGN(4M);

        PROVIDE_HIDDEN (LINK_E = . - OFFSET);

        PROVIDE_HIDDEN (_mempool_e = .);

    } : kern

    PROVIDE_HIDDEN (DSTK_TOP = MMAP_CPU_DSTK + PAGE_SIZE);

    .cpulocal MMAP_CPU_DATA :
    {
        *(SORT_BY_ALIGNMENT (.cpulocal.hot))
        *(SORT_BY_ALIGNMENT (.cpulocal))
    }

    /DISCARD/ :
    {
        *(.comment*)
        *(.note*)
    }
}
