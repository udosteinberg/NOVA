/*
 * Executable and Linkable Format (ELF)
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

#pragma once

#include "compiler.hpp"

class Eh
{
    public:
        uint32  ei_magic;
        uint8   ei_class, ei_data, ei_version, ei_osabi, ei_abiversion, ei_pad[7];
        uint16  type, machine;
        uint32  version;
        mword   entry, ph_offset, sh_offset;
        uint32  flags;
        uint16  eh_size, ph_size, ph_count, sh_size, sh_count, strtab;
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
