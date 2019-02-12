/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "byteorder.hpp"

/*
 * 5.2.3.2: Generic Address Structure (GAS)
 */
class Acpi_gas final
{
    public:
        enum class Asid : uint8_t
        {
            MEM         = 0x0,      // System Memory Space
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

        Asid                    asid    { 0 };  // Address space where the data structure or register exists
        Unaligned_le<uint8_t>   bits;           // Number of bits in the given register (0=non-existent)
        Unaligned_le<uint8_t>   offs;           // Bit offset of the given register at the given address
        Unaligned_le<uint8_t>   accs;           // Access size (0=undefined, 1=8bit, 2=16bit, 3=32bit, 4=64bit)
        Unaligned_le<uint64_t>  addr;           // Address of the data structure or register in the given address space

        bool valid() const { return bits; }

        constexpr Acpi_gas() = default;

        Acpi_gas (Acpi_gas x_blk, uint32_t blk, uint8_t len, unsigned c, unsigned i) : offs { 0 }, accs { 0 }
        {
            if (x_blk.bits) {                   // Extended block
                asid = x_blk.asid;
                bits = static_cast<uint8_t>(x_blk.bits / c);
                addr = x_blk.addr + bits / 8 * i;
            } else if (blk) {                   // Legacy block
                asid = Asid::PIO;
                bits = static_cast<uint8_t>(len * 8 / c);
                addr = blk + bits / 8 * i;
            }
        }
};

static_assert (__is_standard_layout (Acpi_gas) && alignof (Acpi_gas) == 1 && sizeof (Acpi_gas) == 12);
