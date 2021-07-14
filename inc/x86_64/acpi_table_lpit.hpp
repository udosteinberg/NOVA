/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_gas.hpp"
#include "acpi_table.hpp"

/*
 * Low Power Idle Table (LPIT)
 */
class Acpi_table_lpit final : private Acpi_table
{
    private:
        /*
         * State Descriptor
         */
        struct Descriptor
        {
            enum Type : uint32
            {
                NATIVE  = 0,
            };

            Type        type;                           // 0 + n
            uint32      length;                         // 4 + n
        };

        /*
         * MWAIT Entry Trigger Descriptor
         */
        struct Descriptor_native : Descriptor
        {
            enum Flags : uint32
            {
                DISABLED        = BIT (0),
                NO_COUNTER      = BIT (1),
            };

            uint16      id;                             //  8 + n
            uint16      reserved;                       // 10 + n
            Flags       flags;                          // 12 + n
            Acpi_gas    trigger;                        // 16 + n
            uint32      min_residency;                  // 28 + n
            uint32      max_latency;                    // 32 + n
            Acpi_gas    counter;                        // 36 + n
            uint64      counter_freq;                   // 48 + n
                                                        // 56 + n
            void parse() const;
        };

    public:
        void parse() const;
};
