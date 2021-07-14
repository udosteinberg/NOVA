/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi_table.hpp"

/*
 * 8.1: DMA Remapping Description Table (DMAR)
 */
class Acpi_table_dmar final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint8_t>       haw;                        // 36
        Unaligned_le<uint8_t>       flags;                      // 37
        Unaligned_le<uint8_t>       reserved[10];               // 38

        /*
         * 8.3.1: Device Scope Structure
         */
        struct Scope : private Unaligned_le<uint8_t>            // 0
        {
            Unaligned_le<uint8_t>   length;                     // 1
            Unaligned_le<uint16_t>  reserved;                   // 2
            Unaligned_le<uint8_t>   id, b, d, f;                // 4

            enum class Type : uint8_t
            {
                PCI_EP  = 1,                                    // PCI Endpoint Device
                PCI_SH  = 2,                                    // PCI Sub-Hierarchy
                IOAPIC  = 3,                                    // IOAPIC
                HPET    = 4,                                    // HPET (MSI-Capable)
                ACPI    = 5,                                    // ACPI Namespace Device
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Scope) == 1 && sizeof (Scope) == 8);

        /*
         * 8.2: Remapping Structure
         */
        struct Remapping : protected Unaligned_le<uint16_t>     // 0
        {
            Unaligned_le<uint16_t>  length;                     // 2

            enum class Type : uint16_t
            {
                DRHD    = 0,                                    // DMA Remapping Hardware Unit Definition
                RMRR    = 1,                                    // Reserved Memory Region Reporting
                ATSR    = 2,                                    // Root Port ATS Capability Reporting
                RHSA    = 3,                                    // Remapping Hardware Static Affinity
                ANDD    = 4,                                    // ACPI Namespace Device Declaration
                SATC    = 5,                                    // SoC Integrated Address Translation Cache
            };

            auto type() const { return Type { uint16_t { *this } }; }
        };

        static_assert (alignof (Remapping) == 1 && sizeof (Remapping) == 4);

        /*
         * 8.3: DMA Remapping Hardware Unit Definition (DRHD) Structure
         */
        struct Remapping_drhd : public Remapping                // 0
        {
            Unaligned_le<uint8_t>   flags;                      // 4
            Unaligned_le<uint8_t>   reserved;                   // 5
            Unaligned_le<uint16_t>  segment;                    // 6
            Unaligned_le<uint64_t>  phys;                       // 8

            void parse() const;
        };

        static_assert (alignof (Remapping_drhd) == 1 && sizeof (Remapping_drhd) == 16);

        /*
         * 8.4: Reserved Memory Region Reporting (RMRR) Structure
         */
        struct Remapping_rmrr : public Remapping                // 0
        {
            Unaligned_le<uint16_t>  reserved;                   // 4
            Unaligned_le<uint16_t>  segment;                    // 6
            Unaligned_le<uint64_t>  base;                       // 8
            Unaligned_le<uint64_t>  limit;                      // 16

            void parse() const;
        };

        static_assert (alignof (Remapping_rmrr) == 1 && sizeof (Remapping_rmrr) == 24);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_dmar) && alignof (Acpi_table_dmar) == 1 && sizeof (Acpi_table_dmar) == 48);
