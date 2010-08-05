/*
 * Executable and Linkable Format (ELF)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.h"

class Elf_eh
{
    public:
        uint32          ident[4];
        uint16          type;
        uint16          machine;
        uint32          version;
        uint32          entry;
        uint32          ph_offset;
        uint32          sh_offset;
        uint32          flags;
        uint16          eh_size;
        uint16          ph_size;
        uint16          ph_count;
        uint16          sh_size;
        uint16          sh_count;
        uint16          strtab;
};

class Elf_ph
{
    public:
        enum
        {
            PT_NULL         = 0,
            PT_LOAD         = 1,
            PT_DYNAMIC      = 2,
            PT_INTERP       = 3,
            PT_NOTE         = 4,
            PT_SHLIB        = 5,
            PT_PHDR         = 6
        };

        enum
        {
            PF_X            = 0x1,
            PF_W            = 0x2,
            PF_R            = 0x4
        };

        uint32          type;
        uint32          f_offs;
        uint32          v_addr;
        uint32          p_addr;
        uint32          f_size;
        uint32          m_size;
        uint32          flags;
        uint32          align;
};
