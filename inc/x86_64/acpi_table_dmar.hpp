/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi_table.hpp"

/*
 * 8.1: DMA Remapping Description Table (DMAR)
 */
class Acpi_table_dmar final
{
    private:
        enum Flags : uint8
        {
            INTR_REMAPPING      = BIT (0),
            X2APIC_OPT_OUT      = BIT (1),
            DMA_CTRL_OPT_IN     = BIT (2),
        };

        Acpi_table  table;                              //  0
        uint8       haw;                                // 36
        Flags       flags;                              // 37
        uint8       reserved[10];                       // 38

        /*
         * 8.3.1: Device Scope Structure
         */
        struct Scope
        {
            enum Type : uint8
            {
                PCI_EP  = 1,                            // PCI Endpoint Device
                PCI_SH  = 2,                            // PCI Sub-Hierarchy
                IOAPIC  = 3,                            // IOAPIC
                HPET    = 4,                            // HPET (MSI-Capable)
                ACPI    = 5,                            // ACPI Namespace Device
            };

            Type        type;                           // 0 + n
            uint8       length;                         // 1 + n
            uint16      reserved;                       // 2 + n
            uint8       id, b, d, f;                    // 4 + n

            inline auto dev() const { return static_cast<uint16>(b << 8 | d << 3 | f); }
        };

        /*
         * 8.2: Remapping Structure
         */
        struct Remapping
        {
            enum Type : uint16
            {
                DRHD    = 0,                            // DMA Remapping Hardware Unit Definition
                RMRR    = 1,                            // Reserved Memory Region Reporting
                ATSR    = 2,                            // Root Port ATS Capability Reporting
                RHSA    = 3,                            // Remapping Hardware Static Affinity
                ANDD    = 4,                            // ACPI Namespace Device Declaration
                SATC    = 5,                            // SoC Integrated Address Translation Cache
            };

            Type        type;                           //  0 + n
            uint16      length;                         //  2 + n
        };

        /*
         * 8.3: DMA Remapping Hardware Unit Definition (DRHD) Structure
         */
        struct Remapping_drhd : Remapping
        {
            enum Flags : uint8
            {
                INCLUDE_PCI_ALL = BIT (0),
            };

            Flags       flags;                          //  4 + n
            uint8       reserved;                       //  5 + n
            uint16      segment;                        //  6 + n
            uint64      phys;                           //  8 + n

            void parse() const;
        };

        /*
         * 8.4: Reserved Memory Region Reporting (RMRR) Structure
         */
        struct Remapping_rmrr : Remapping
        {
            uint16      reserved;                       //  4 + n
            uint16      segment;                        //  6 + n
            uint64      base;                           //  8 + n
            uint64      limit;                          // 16 + n

            void parse() const;
        };

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_dmar) && sizeof (Acpi_table_dmar) == 48);
