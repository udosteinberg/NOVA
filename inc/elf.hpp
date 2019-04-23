/*
 * Executable and Linkable Format (ELF)
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

#include "macros.hpp"
#include "signature.hpp"

struct Eh
{
    enum class Class : uint8_t
    {
        E32         = 0x1,          // 32-bit objects
        E64         = 0x2,          // 64-bit objects
    };

    enum class Data : uint8_t
    {
        LSB         = 0x1,          // Little Endian
        MSB         = 0x2,          // Big Endian
    };

    enum class Type : uint16_t
    {
        ET_NONE     = 0x0,          // No file type
        ET_REL      = 0x1,          // Relocatable file
        ET_EXEC     = 0x2,          // Executable file
        ET_DYN      = 0x3,          // Shared object file
        ET_CORE     = 0x4,          // Core file
    };

    enum class Machine : uint16_t
    {
        X86_64      = 0x3e,
        AARCH64     = 0xb7,
        RISCV       = 0xf3,
    };

    uint32_t        ei_magic;
    Class           ei_class;
    Data            ei_data;
    uint8_t         ei_version, ei_osabi, ei_abiversion, ei_pad[7];
    Type            type;
    Machine         machine;
    uint32_t        version;
    uintptr_t       entry, ph_offset, sh_offset;
    uint32_t        flags;
    uint16_t        eh_size, ph_size, ph_count, sh_size, sh_count, strtab;

    [[nodiscard]] constexpr bool valid (Machine m) const
    {
        return ei_magic == Signature::u32 ("\x7f""ELF") && ei_class == Class::E64 && ei_data == Data::LSB && ei_version == 1 && type == Type::ET_EXEC && machine == m;
    }
};

static_assert (__is_standard_layout (Eh) && alignof (Eh) == 8 && sizeof (Eh) == 64);

struct Ph
{
    enum class Type : uint32_t
    {
        PT_NULL     = 0x0,
        PT_LOAD     = 0x1,
        PT_DYNAMIC  = 0x2,
        PT_INTERP   = 0x3,
        PT_NOTE     = 0x4,
        PT_SHLIB    = 0x5,
        PT_PHDR     = 0x6,
        PT_TLS      = 0x7,
    };

    enum Perm
    {
        X           = BIT (0),
        W           = BIT (1),
        R           = BIT (2),
        ALL         = R | W | X,
    };

    Type            type;
    uint32_t        flags;
    uint64_t        f_offs, v_addr, p_addr, f_size, m_size, align;
};

static_assert (__is_standard_layout (Ph) && alignof (Ph) == 8 && sizeof (Ph) == 56);
