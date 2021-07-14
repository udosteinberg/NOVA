/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_gas.hpp"
#include "acpi_table.hpp"

/*
 * Low Power Idle Table (LPIT)
 */
class Acpi_table_lpit final
{
    private:
        Acpi_table  table;                              //  0

        /*
         * State Descriptor
         */
        struct Descriptor
        {
            enum Type : uint32_t
            {
                NATIVE  = 0,
            };

            Type        type;                           // 0 + n
            uint32_t    length;                         // 4 + n
        };

        static_assert (__is_standard_layout (Descriptor) && sizeof (Descriptor) == 8);

        /*
         * MWAIT Entry Trigger Descriptor
         */
        struct Descriptor_native : Descriptor
        {
            enum Flags : uint32_t
            {
                DISABLED        = BIT (0),
                NO_COUNTER      = BIT (1),
            };

            uint16_t    id;                             //  8 + n
            uint16_t    reserved;                       // 10 + n
            Flags       flags;                          // 12 + n
            Acpi_gas    trigger;                        // 16 + n
            uint32_t    min_residency;                  // 28 + n
            uint32_t    max_latency;                    // 32 + n
            Acpi_gas    counter;                        // 36 + n
            uint64_t    counter_freq;                   // 48 + n

            void parse() const;
        };

        static_assert (sizeof (Descriptor_native) == 56);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_lpit) && sizeof (Acpi_table_lpit) == 36);
