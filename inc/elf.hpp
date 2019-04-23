/*
 * Executable and Linkable Format (ELF)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "types.hpp"

class Eh
{
    private:
        enum Elf_data : uint8
        {
            ED_LSB      = 1,
        };

        enum Elf_type : uint16
        {
            ET_EXEC     = 2,
        };

    public:
        enum Elf_class : uint8
        {
            EC_32       = 1,
            EC_64       = 2,
        };

        enum Elf_machine : uint16
        {
            EM_386      = 3,
            EM_X86_64   = 62,
            EM_AARCH64  = 183,
        };

        bool valid (Elf_class c, Elf_machine m) const
        {
            return ei_magic == 0x464c457f && ei_class == c && ei_data == ED_LSB && type == ET_EXEC && machine == m;
        }

        uint32          ei_magic;
        Elf_class       ei_class;
        Elf_data        ei_data;
        uint8           ei_version, ei_osabi, ei_abiversion, ei_pad[7];
        Elf_type        type;
        Elf_machine     machine;
        uint32          version;
        mword           entry, ph_offset, sh_offset;
        uint32          flags;
        uint16          eh_size, ph_size, ph_count, sh_size, sh_count, strtab;
};

class Ph32
{
    public:
        uint32  type, f_offs, v_addr, p_addr, f_size, m_size, flags, align;
};

class Ph64
{
    public:
        uint32  type, flags;
        uint64  f_offs, v_addr, p_addr, f_size, m_size, align;
};
