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

#include "acpi_table.hpp"

/*
 * 5.2.10: Firmware ACPI Control Structure (FACS)
 */
class Acpi_table_facs final : private Acpi_header
{
    private:
        uint32      hardware;                       //  8 (1.0)
        uint32      wake32;                         // 12 (1.0)
        uint32      global_lock;                    // 16 (1.0)
        uint32      flags;                          // 20 (1.0)
        uint64      wake64;                         // 24 (2.0)
        uint8       version;                        // 32 (2.0)
        uint8       reserved1[3];                   // 33
        uint32      ospm_flags;                     // 36 (4.0)
        uint8       reserved2[24];                  // 40

    public:
        void set_wake (uint32 w)
        {
            if (signature != sig_value ("FACS"))
                return;

            wake32 = w;
            wake64 = 0;
        }

        void parse() const;
};
