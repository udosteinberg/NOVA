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
 * Low Power Idle Table (LPIT)
 */
class Acpi_table_lpit final
{
    private:
        Acpi_table                  table;                      // 0

        /*
         * State Descriptor
         */
        struct Descriptor : protected Unaligned_le<uint32_t>    // 0
        {
            Unaligned_le<uint32_t>  length;                     // 4

            enum class Type : uint32_t
            {
                NATIVE  = 0,
            };

            auto type() const { return Type { uint32_t { *this } }; }
        };

        static_assert (alignof (Descriptor) == 1 && sizeof (Descriptor) == 8);

        /*
         * MWAIT Entry Trigger Descriptor
         */
        struct Descriptor_native : public Descriptor            // 0
        {
            Unaligned_le<uint16_t>  id;                         // 8
            Unaligned_le<uint16_t>  reserved;                   // 10
            Unaligned_le<uint32_t>  flags;                      // 12
            Acpi_gas                trigger;                    // 16
            Unaligned_le<uint32_t>  min_residency;              // 28
            Unaligned_le<uint32_t>  max_latency;                // 32
            Acpi_gas                counter;                    // 36
            Unaligned_le<uint64_t>  counter_freq;               // 48

            void parse() const;
        };

        static_assert (alignof (Descriptor_native) == 1 && sizeof (Descriptor_native) == 56);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_lpit) && alignof (Acpi_table_lpit) == 1 && sizeof (Acpi_table_lpit) == 36);
