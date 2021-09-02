/*
 * Linker Script
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

#include "arch.hpp"
#include "memory.hpp"

OUTPUT_ARCH(BFD_ARCH)
OUTPUT_FORMAT(BFD_FORMAT)
ENTRY(__init_bsp)

PHDRS
{
    init    PT_LOAD;
    kern    PT_LOAD;
}

SECTIONS
{
    .init LOAD_ADDR :
    {
        *(.init .init.*)
        KEEP (*(.multiboot))
    } : init

    .stack :
    {
        . = ALIGN(4K); PROVIDE_HIDDEN (STACK = .);
    } : init

    .text . + OFFSET : AT (ADDR (.text) - OFFSET)
    {
        *(.text.hot .text.hot.*)
        *(.text .text.* .gnu.linkonce.t.*)
    } : kern

    .rodata : AT (ADDR (.rodata) - OFFSET)
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } : kern

    .patch : AT (ADDR (.patch) - OFFSET)
    {
        PROVIDE_HIDDEN (PATCH_S = .);
        KEEP (*(.patch.data))
        PROVIDE_HIDDEN (PATCH_E = .);
        KEEP (*(.patch.code))
    } : kern

    .init_array : AT (ADDR (.init_array) - OFFSET)
    {
        PROVIDE_HIDDEN (CTORS_L = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.65534)))
        PROVIDE_HIDDEN (CTORS_C = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.65533)))
        PROVIDE_HIDDEN (CTORS_S = .);
        KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE_HIDDEN (CTORS_E = .);
    } : kern

    .data : AT (ADDR (.data) - OFFSET)
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } : kern

    .bss : AT (ADDR (.bss) - OFFSET)
    {
        PROVIDE_HIDDEN (ZERO_HPAS = . - OFFSET);

        *(.bss .bss.* .gnu.linkonce.b.*)

        . = ALIGN(4K);

        PROVIDE_HIDDEN (KMEM_HVAS = .);

        PROVIDE_HIDDEN (PAGE_H = .); . += 4K;

        PROVIDE_HIDDEN (PTAB_HVAS = .);
        PROVIDE_HIDDEN (PT3S_HPAS = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (PT2H_HPAS = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (PT1H_HPAS = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (PT2L_HPAS = . - OFFSET); . += 4K;
        PROVIDE_HIDDEN (PT1L_HPAS = . - OFFSET); . += 4K;

        PROVIDE_HIDDEN (KMEM_HVAF = .);

        . = ALIGN(16M);

        PROVIDE_HIDDEN (KMEM_HVAE = .);

        PROVIDE_HIDDEN (ZERO_HPAE = . - OFFSET);

    } : kern

    PROVIDE_HIDDEN (NOVA_HPAS = LOAD_ADDR);
    PROVIDE_HIDDEN (NOVA_HPAE = . - OFFSET);

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
