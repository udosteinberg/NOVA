/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#pragma pack(1)

/*
 * 5.2.3.2: Generic Address Structure (GAS)
 */
class Acpi_gas
{
    public:
        enum class Asid : uint8
        {
            MEMORY      = 0x0,
            IO          = 0x1,
            PCI_CONFIG  = 0x2,
            EC          = 0x3,
            SMBUS       = 0x4,
            FIXED       = 0x7f
        };

        Asid    asid;       // ASID
        uint8   bits;       // Register Size (bits)
        uint8   offset;     // Register Offset
        uint8   access;     // Access Size
        uint64  addr;       // Register Address

        void init (Asid reg_asid, unsigned reg_bytes, uint64 reg_addr)
        {
            asid = reg_asid;
            bits = static_cast<uint8>(reg_bytes * 8);
            addr = reg_addr;
        }
};

#pragma pack()
