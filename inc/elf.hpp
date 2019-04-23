/*
 * Executable and Linkable Format (ELF)
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

#pragma once

#include "signature.hpp"

struct Eh
{
    enum class Class : uint8_t
    {
        E32         = 0x1,
        E64         = 0x2,
    };

    enum class Data : uint8_t
    {
        LSB         = 0x1,
        MSB         = 0x2,
    };

    enum class Type : uint16_t
    {
        EXEC        = 0x2,
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

    [[nodiscard]] bool valid (Machine m) const
    {
        return ei_magic == Signature::value ("\x7f""ELF") && ei_class == Class::E64 && ei_data == Data::LSB && ei_version == 1 && type == Type::EXEC && machine == m;
    }
};

struct Ph
{
    uint32_t        type, flags;
    uint64_t        f_offs, v_addr, p_addr, f_size, m_size, align;
};
