/*
 * Linker Script
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "memory.h"

#ifdef __i386__
OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
#else
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")
#endif
ENTRY(__start_bsp)

PHDRS
{
    init    PT_LOAD;
    kern    PT_LOAD;
}

SECTIONS
{
    .init LOAD_ADDR :
    {
        *(.init)
        *(.initdata)
    } : init = 0x90909090

    . = ALIGN(4K) + 4K; PROVIDE (STACK = .);

    PROVIDE (LOAD_E = .);
    OFFSET = LINK_ADDR - ALIGN (4M);
    PROVIDE (LINK_P = LINK_ADDR - OFFSET);

    .text LINK_ADDR : AT (ADDR (.text) - OFFSET)
    {
        *(.text.hot .text.hot.*)
        *(.text .text.* .gnu.linkonce.t.*)
    } : kern = 0x90909090

    .rodata : AT (ADDR (.rodata) - OFFSET)
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } : kern

    .fixup : AT (ADDR (.fixup) - OFFSET)
    {
        PROVIDE (FIXUP_S = .);
        KEEP (*(.fixup))
        PROVIDE (FIXUP_E = .);
    } : kern

    .init_array : AT (ADDR (.init_array) - OFFSET)
    {
        PROVIDE (CTORS_L = .);
        KEEP (SORT_BY_INIT_PRIORITY(*)(.init_array.65534 .ctors.00001))
        PROVIDE (CTORS_C = .);
        KEEP (SORT_BY_INIT_PRIORITY(*)(.init_array.65533 .ctors.00002))
        PROVIDE (CTORS_G = .);
        KEEP (SORT_BY_INIT_PRIORITY(*)(.init_array.* .ctors.*))
        KEEP (*(.init_array .ctors))
        PROVIDE (CTORS_E = .);
    } : kern

    .data : AT (ADDR (.data) - OFFSET)
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } : kern

    .bss : AT (ADDR (.bss) - OFFSET)
    {
        *(.bss .bss.* .gnu.linkonce.b.*)

        . = ALIGN(4K);
        _mempool_l = .; PROVIDE (_mempool_p = _mempool_l - OFFSET);

        PROVIDE (PAGE_0 = .); PROVIDE (FRAME_0 = . - OFFSET); . += 4K;
        PROVIDE (PAGE_1 = .); PROVIDE (FRAME_1 = . - OFFSET); . += 4K;
        PROVIDE (PAGE_H = .); PROVIDE (FRAME_H = . - OFFSET); . += 4K;

        PROVIDE (PDBR  = . - OFFSET);
#ifdef __i386__
        PROVIDE (LVL2L = . - OFFSET);
        PROVIDE (LVL2H = . - OFFSET); . += 4K;
#else
        PROVIDE (LVL4  = . - OFFSET); . += 4K;
        PROVIDE (LVL3L = . - OFFSET); . += 4K;
        PROVIDE (LVL3H = . - OFFSET); . += 4K;
        PROVIDE (LVL2L = . - OFFSET); . += 4K;
        PROVIDE (LVL2H = . - OFFSET); . += 4K;
#endif
        _mempool_f = .;

        . += 16M;
        . = ALIGN(4M);

        PROVIDE (LINK_E = . - OFFSET);

        PROVIDE (_mempool_e = .);

    } : kern

    .cpulocal CPU_LOCAL_DATA :
    {
        SORT_BY_ALIGNMENT (*)(.cpulocal.hot)
        SORT_BY_ALIGNMENT (*)(.cpulocal)
    }

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment*)
    }
}
