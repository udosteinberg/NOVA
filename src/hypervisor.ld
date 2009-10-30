/*
 * Linker Script
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
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

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(__start_bsp)

PHDRS
{
    smxe    PT_LOAD;
    init    PT_LOAD;
    kern    PT_LOAD;
}

SECTIONS
{
    .smxe LOAD_ADDR :
    {
        PROVIDE (PDP = .); . += 4K;
        PROVIDE (PDE = .); . += 4K;
        PROVIDE (PTE = .); . += 4K;
        . += 4K; PROVIDE (STACK = .);
    } : smxe

    .init INIT_ADDR :
    {
        *(.init)
        *(.initdata)
    } : init

    . = ALIGN(4K);
    PROVIDE (LOAD_SIZE = . - LOAD_ADDR);
    PROVIDE (INIT_SIZE = . - INIT_ADDR);

    OFFSET = LINK_ADDR - ALIGN (4M);
    PROVIDE (LINK_PHYS = LINK_ADDR - OFFSET);

    .text LINK_ADDR : AT (ADDR (.text) - OFFSET)
    {
        *(.text.hot)
        *(.text .text.* .gnu.linkonce.t.*)
    } : kern

    .rodata : AT (ADDR (.rodata) - OFFSET)
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } : kern

    .ctors : AT (ADDR (.ctors) - OFFSET)
    {
        PROVIDE (CTORS_S = .);
        KEEP (*(.ctors))
        KEEP (*(SORT(.ctors.*)))
        PROVIDE (CTORS_E = .);
    } : kern

    .data : AT (ADDR (.data) - OFFSET)
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } : kern

    PROVIDE (HASH_SIZE = ALIGN(4K) - LINK_ADDR);

    .bss : AT (ADDR (.bss) - OFFSET)
    {
        *(.bss .bss.* .gnu.linkonce.b.*)

        . = ALIGN(4K);
        _mempool_l = .; PROVIDE (_mempool_p = _mempool_l - OFFSET);

        PROVIDE (PAGE_0 = .); PROVIDE (FRAME_0 = PAGE_0 - OFFSET);
        . += 4K;
        PROVIDE (PAGE_1 = .); PROVIDE (FRAME_1 = PAGE_1 - OFFSET);
        . += 4K;
        PROVIDE (PAGE_H = .); PROVIDE (FRAME_H = PAGE_H - OFFSET);
        . += 4K;

        PROVIDE (_master_l = .); PROVIDE (_master_p = _master_l - OFFSET);
        . += 4K;
        PROVIDE (_tables_l = .); PROVIDE (_tables_p = _tables_l - OFFSET);
        . += 64K;
        _mempool_f = .;

        . += 4M;
        . = ALIGN(4M);
        PROVIDE (LINK_SIZE = . - LINK_ADDR);

        PROVIDE (_mempool_e = .);

    } : kern

    .cpulocal CPULC_ADDR :
    {
        SORT_BY_ALIGNMENT (*)(.cpulocal.hot)
        SORT_BY_ALIGNMENT (*)(.cpulocal)
    }

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment*)
    }

    // XXX: Remove
    PROVIDE (boot_lock_p = boot_lock - OFFSET);
}
