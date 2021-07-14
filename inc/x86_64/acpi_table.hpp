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

#include "acpi_gas.hpp"
#include "checksum.hpp"
#include "macros.hpp"
#include "signature.hpp"

/*
 * 5.2.6: System Description Table Header
 */
struct Acpi_header final
{
    Unaligned_le<uint32_t>  signature;                      // 0    1.0+
    Unaligned_le<uint32_t>  length;                         // 4    1.0+
};

static_assert (__is_standard_layout (Acpi_header) && alignof (Acpi_header) == 1 && sizeof (Acpi_header) == 8);

struct Acpi_table final
{
    Acpi_header             header;                         // 0    1.0+
    Unaligned_le<uint8_t>   revision;                       // 8    1.0+
    Unaligned_le<uint8_t>   checksum;                       // 9    1.0+
    char                    oem_id[6];                      // 10   1.0+
    char                    oem_table_id[8];                // 16   1.0+
    Unaligned_le<uint32_t>  oem_revision;                   // 24   1.0+
    char                    creator_id[4];                  // 28   1.0+
    Unaligned_le<uint32_t>  creator_revision;               // 32   1.0+

    bool validate (uint64_t) const;

    static void const *map (uint64_t, unsigned);

    static bool consume (uint64_t p)
    {
        auto const t { static_cast<Acpi_table const *>(map (p, 1)) };

        return t && t->validate (p);
    }
};

static_assert (__is_standard_layout (Acpi_table) && alignof (Acpi_table) == 1 && sizeof (Acpi_table) == 36);
