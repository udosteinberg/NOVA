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
 * 5.2.5: Root System Description Pointer (RSDP)
 */
class Acpi_table_rsdp final
{
    private:
        Unaligned_le<uint64_t>      signature;                  // 0    1.0+
        Unaligned_le<uint8_t>       checksum;                   // 8    1.0+
        char                        oem_id[6];                  // 9    1.0+
        Unaligned_le<uint8_t>       revision;                   // 15   1.0+
        Unaligned_le<uint32_t>      rsdt;                       // 16   1.0+
        Unaligned_le<uint32_t>      length;                     // 20   2.0+
        Unaligned_le<uint64_t>      xsdt;                       // 24   2.0+
        Unaligned_le<uint8_t>       extended_checksum;          // 32   2.0+
        Unaligned_le<uint8_t>       reserved[3];                // 33   2.0+

    public:
        [[nodiscard]] bool valid() const
        {
            return signature == Signature::u64 ("RSD PTR ") && Checksum::additive (reinterpret_cast<uint8_t const *>(this), revision > 1 ? uint32_t { length } : uint32_t { 20 }) == 0;
        }

        [[nodiscard]] bool parse() const
        {
            return valid() && Acpi_table::consume (revision > 1 ? xsdt : rsdt);
        }
};

static_assert (__is_standard_layout (Acpi_table_rsdp) && alignof (Acpi_table_rsdp) == 1 && sizeof (Acpi_table_rsdp) == 36);
