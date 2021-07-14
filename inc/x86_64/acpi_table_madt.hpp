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
 * 5.2.12: Multiple APIC Description Table (MADT)
 */
class Acpi_table_madt final
{
    private:
        enum Flags : uint32
        {
            PCAT_COMPAT         = BIT (0),
        };

        Acpi_table  table;                              //  0
        uint32      phys;                               // 36
        Flags       flags;                              // 40

        /*
         * Each Controller struct is only 32bit (but not 64bit) aligned. This is because
         * the first struct is at offset n=44 and each struct has a multiple-of-4 size.
         */
        struct Controller
        {
            enum Type : uint8
            {
                LAPIC   = 0,                            // Local APIC
                IOAPIC  = 1,                            // I/O APIC
                X2APIC  = 9,                            // Local x2APIC
            };

            Type        type;                           //  0 + n
            uint8       length;                         //  1 + n
        };

        /*
         * 5.2.12.2: Local APIC Structure
         */
        struct Controller_lapic : Controller
        {
            enum Flags : uint32
            {
                ENABLED             = BIT (0),
                ONLINE_CAPABLE      = BIT (1),
            };

            uint8       acpi_uid;                       //  2 + n
            uint8       id;                             //  3 + n
            Flags       flags;                          //  4 + n
                                                        //  8 + n
            void parse() const;
        };

        /*
         * 5.2.12.3: I/O APIC Structure
         */
        struct Controller_ioapic : Controller
        {
            uint8       id;                             //  2 + n
            uint8       reserved;                       //  3 + n
            uint32      phys;                           //  4 + n
            uint32      gsi;                            //  8 + n
                                                        // 12 + n
            void parse() const;
        };

        /*
         * 5.2.12.12: x2APIC Structure
         */
        struct Controller_x2apic : Controller
        {
            using Flags = Controller_lapic::Flags;

            uint16      reserved;                       //  2 + n
            uint32      id;                             //  4 + n
            Flags       flags;                          //  8 + n
            uint32      acpi_uid;                       // 12 + n
                                                        // 16 + n
            void parse() const;
        };

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_madt) && sizeof (Acpi_table_madt) == 44);
