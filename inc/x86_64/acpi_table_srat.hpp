/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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
 * 5.2.16: System Resource Affinity Table (SRAT)
 */
class Acpi_table_srat final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      reserved1;                  // 36
        Unaligned_le<uint64_t>      reserved2;                  // 40

        struct Affinity : protected Unaligned_le<uint8_t>       // 0
        {
            Unaligned_le<uint8_t>   length;                     // 1

            enum class Type : uint8_t
            {
                LAPIC   = 0,                                    // Local APIC
                MEMORY  = 1,                                    // Memory
                X2APIC  = 2,                                    // Local x2APIC
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Affinity) == 1 && sizeof (Affinity) == 2);

        /*
         * 5.2.16.1: Local APIC Affinity Structure
         */
        struct Affinity_lapic final : public Affinity           // 0
        {
            Unaligned_le<uint8_t>   pxd0;                       // 2
            Unaligned_le<uint8_t>   id;                         // 3
            Unaligned_le<uint32_t>  flags;                      // 4
            Unaligned_le<uint8_t>   eid;                        // 8
            Unaligned_le<uint8_t>   pxd1, pxd2, pxd3;           // 9
            Unaligned_le<uint32_t>  clock;                      // 12

            void parse() const;
        };

        static_assert (alignof (Affinity_lapic) == 1 && sizeof (Affinity_lapic) == 16);

        /*
         * 5.2.16.2: Memory Affinity Structure
         */
        struct Affinity_memory final : public Affinity          // 0
        {
            Unaligned_le<uint32_t>  pxd;                        // 2
            Unaligned_le<uint16_t>  reserved1;                  // 6
            Unaligned_le<uint64_t>  base;                       // 8
            Unaligned_le<uint64_t>  size;                       // 16
            Unaligned_le<uint32_t>  reserved2;                  // 24
            Unaligned_le<uint32_t>  flags;                      // 28
            Unaligned_le<uint64_t>  reserved3;                  // 32

            void parse() const;
        };

        static_assert (alignof (Affinity_memory) == 1 && sizeof (Affinity_memory) == 40);

        /*
         * 5.2.16.3: x2APIC Affinity Structure
         */
        struct Affinity_x2apic final : public Affinity          // 0
        {
            Unaligned_le<uint16_t>  reserved1;                  // 2
            Unaligned_le<uint32_t>  pxd;                        // 4
            Unaligned_le<uint32_t>  id;                         // 8
            Unaligned_le<uint32_t>  flags;                      // 12
            Unaligned_le<uint32_t>  clock;                      // 16
            Unaligned_le<uint32_t>  reserved2;                  // 20

            void parse() const;
        };

        static_assert (alignof (Affinity_x2apic) == 1 && sizeof (Affinity_x2apic) == 24);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_srat) && alignof (Acpi_table_srat) == 1 && sizeof (Acpi_table_srat) == 48);
