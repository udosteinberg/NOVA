/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
 * Memory Mapped Configuration Space Description Table (MCFG)
 */
class Acpi_table_mcfg final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      reserved[2];                // 36

        struct Segment
        {
            Unaligned_le<uint64_t>  phys_base;                  // 0
            Unaligned_le<uint16_t>  seg;                        // 8
            Unaligned_le<uint8_t>   sbn;                        // 10
            Unaligned_le<uint8_t>   ebn;                        // 11
            Unaligned_le<uint32_t>  reserved;                   // 12

            void parse (char const (&)[6], char const (&)[8]) const;
        };

        static_assert (__is_standard_layout (Segment) && alignof (Segment) == 1 && sizeof (Segment) == 16);

        static constexpr struct
        {
            char const *oem;
            char const *tbl;
            uint64_t    seg;
        } quirk[] =
        {
            { "NVIDIA", "TEGRA194", BIT64_RANGE (63, 0) },
        };

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_mcfg) && alignof (Acpi_table_mcfg) == 1 && sizeof (Acpi_table_mcfg) == 44);
