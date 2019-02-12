/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
class Acpi_gas final
{
    public:
        enum class Asid : uint8_t
        {
            MMIO        = 0x0,      // System Memory Space
            PIO         = 0x1,      // System I/O Space
            PCI_CFG     = 0x2,      // PCI Configuration Space
            EC          = 0x3,      // Embedded Controller
            SMBUS       = 0x4,      // SMBus
            CMOS        = 0x5,      // System CMOS
            PCI_BAR     = 0x6,      // PCI BAR
            IPMI        = 0x7,      // IPMI
            GPIO        = 0x8,      // General Purpose I/O
            SERIAL      = 0x9,      // Generic Serial Bus
            PCC         = 0xa,      // Platform Communication Channel
            PRM         = 0xb,      // Platform Runtime Mechanism
            FFH         = 0x7f,     // Functional Fixed Hardware
        };

        Asid        asid    { 0 };  // Address space where the data structure or register exists
        uint8_t     bits    { 0 };  // Number of bits in the given register (0=non-existent)
        uint8_t     offs    { 0 };  // Bit offset of the given register at the given address
        uint8_t     size    { 0 };  // Access size (0=undefined, 1=8bit, 2=16bit, 3=32bit, 4=64bit)
        uint64_t    addr    { 0 };  // Address of the data structure or register in the given address space

        inline bool valid() const { return bits; }

        Acpi_gas() = default;

        Acpi_gas (Acpi_gas x, uint64_t a, uint8_t l, unsigned c, unsigned i) : offs (0), size (0)
        {
            if (x.bits) {           // Use extended block
                asid = x.asid;
                addr = x.addr;
            } else if (a) {         // Use legacy block
                asid = Asid::PIO;
                addr = a;
            } else                  // Non-existent
                return;

            // Note: x.bits can only encode values up to 255, but l * 8 may be larger (for GPEs)
            bits = static_cast<uint8_t>((l * 8 > x.bits ? l * 8 : x.bits) / c);
            addr += bits / 8 * i;
        }
};

#pragma pack()
