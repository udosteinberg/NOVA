/*
 * Executable and Linkable Format (ELF)
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

#pragma once

#include "compiler.hpp"
#include "types.hpp"

struct Eh
{
    enum Elf_class : uint8
    {
        EC_32       = 0x1,
        EC_64       = 0x2,
    };

    enum Elf_data : uint8
    {
        ED_LSB      = 0x1,
        ED_MSB      = 0x2,
    };

    enum Elf_type : uint16
    {
        ET_EXEC     = 0x2,
    };

    enum Elf_machine : uint16
    {
        EM_X86_64   = 0x3e,
        EM_AARCH64  = 0xb7,
        EM_RISCV    = 0xf3,
    };

    uint32          ei_magic;
    Elf_class       ei_class;
    Elf_data        ei_data;
    uint8           ei_version, ei_osabi, ei_abiversion, ei_pad[7];
    Elf_type        type;
    Elf_machine     machine;
    uint32          version;
    uintptr_t       entry, ph_offset, sh_offset;
    uint32          flags;
    uint16          eh_size, ph_size, ph_count, sh_size, sh_count, strtab;

    bool valid (Elf_machine m) const
    {
        return ei_magic == 0x464c457f && ei_class == EC_64 && ei_data == ED_LSB && ei_version == 1 && type == ET_EXEC && machine == m;
    }
};

struct Ph
{
    uint32          type, flags;
    uint64          f_offs, v_addr, p_addr, f_size, m_size, align;
};
