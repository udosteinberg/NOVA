/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
 * 5.2.10: Firmware ACPI Control Structure (FACS)
 */
class Acpi_table_facs final
{
    private:
        Acpi_header header;                         //  0 (1.0)
        uint32_t    hardware;                       //  8 (1.0)
        uint32_t    wake32;                         // 12 (1.0)
        uint32_t    global_lock;                    // 16 (1.0)
        uint32_t    flags;                          // 20 (1.0)
        uint64_t    wake64;                         // 24 (2.0)
        uint8_t     version;                        // 32 (2.0)
        uint8_t     reserved1[3];                   // 33
        uint32_t    ospm_flags;                     // 36 (4.0)
        uint8_t     reserved2[24];                  // 40

    public:
        void set_wake (uint32_t w)
        {
            if (header.signature != Signature::value ("FACS"))
                return;

            wake32 = w;
            wake64 = 0;
        }

        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_facs) && sizeof (Acpi_table_facs) == 64);
