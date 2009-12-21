/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "types.h"

/*
 * Generic Address Structure (5.2.3.1)
 */
class Acpi_gas
{
    public:
        uint8   asid;       // ASID
        uint8   bits;       // Register Size (bits)
        uint8   offset;     // Register Offset
        uint8   access;     // Access Size
        uint64  addr;       // Register Address

        enum Asid
        {
            MEMORY      = 0x0,
            IO          = 0x1,
            PCI_CONFIG  = 0x2,
            EC          = 0x3,
            SMBUS       = 0x4,
            FIXED       = 0x7f
        };

        void init (Asid reg_asid, unsigned reg_bytes, uint64 reg_addr)
        {
            asid = reg_asid;
            bits = static_cast<uint8>(reg_bytes * 8);
            addr = reg_addr;
        }
};
