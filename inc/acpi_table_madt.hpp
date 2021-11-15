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
 * 5.2.12: Multiple APIC Description Table (MADT)
 */
class Acpi_table_madt final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      phys;                       // 36
        Unaligned_le<uint32_t>      flags;                      // 40

        struct Controller : protected Unaligned_le<uint8_t>     // 0
        {
            Unaligned_le<uint8_t>   length;                     // 1

            enum class Type : uint8_t
            {
                LAPIC   = 0,                                    // Local APIC
                IOAPIC  = 1,                                    // I/O APIC
                X2APIC  = 9,                                    // Local x2APIC
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Controller) == 1 && sizeof (Controller) == 2);

        /*
         * 5.2.12.2: Local APIC Structure
         */
        struct Controller_lapic : public Controller             // 0
        {
            Unaligned_le<uint8_t>   uid;                        // 2
            Unaligned_le<uint8_t>   id;                         // 3
            Unaligned_le<uint32_t>  flags;                      // 4

            void parse() const;
        };

        static_assert (alignof (Controller_lapic) == 1 && sizeof (Controller_lapic) == 8);

        /*
         * 5.2.12.3: I/O APIC Structure
         */
        struct Controller_ioapic : public Controller            // 0
        {
            Unaligned_le<uint8_t>   id;                         // 2
            Unaligned_le<uint8_t>   reserved;                   // 3
            Unaligned_le<uint32_t>  phys;                       // 4
            Unaligned_le<uint32_t>  gsi;                        // 8

            void parse() const;
        };

        static_assert (alignof (Controller_ioapic) == 1 && sizeof (Controller_ioapic) == 12);

        /*
         * 5.2.12.12: x2APIC Structure
         */
        struct Controller_x2apic : public Controller            // 0
        {
            Unaligned_le<uint16_t>  reserved;                   // 2
            Unaligned_le<uint32_t>  id;                         // 4
            Unaligned_le<uint32_t>  flags;                      // 8
            Unaligned_le<uint32_t>  uid;                        // 12

            void parse() const;
        };

        static_assert (alignof (Controller_x2apic) == 1 && sizeof (Controller_x2apic) == 16);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_madt) && alignof (Acpi_table_madt) == 1 && sizeof (Acpi_table_madt) == 44);
