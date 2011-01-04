/*
 * Executable and Linkable Format (ELF)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
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

class Eh
{
    public:
        uint32          ei_magic;
        uint8           ei_class, ei_data, ei_version, ei_pad[9];
        uint16          type, machine;
        uint32          version;
        mword           entry, ph_offset, sh_offset;
        uint32          flags;
        uint16          eh_size, ph_size, ph_count, sh_size, sh_count, strtab;
};

class Ph
{
    public:
        enum
        {
            PT_NULL     = 0,
            PT_LOAD     = 1,
            PT_DYNAMIC  = 2,
            PT_INTERP   = 3,
            PT_NOTE     = 4,
            PT_SHLIB    = 5,
            PT_PHDR     = 6,
        };

        enum
        {
            PF_X        = 0x1,
            PF_W        = 0x2,
            PF_R        = 0x4,
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
