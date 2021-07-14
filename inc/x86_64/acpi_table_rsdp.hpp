/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "checksum.hpp"
#include "signature.hpp"

/*
 * 5.2.5: Root System Description Pointer (RSDP)
 */
class Acpi_table_rsdp final
{
    friend class Acpi_arch;

    private:
        uint32_t    signature_lo, signature_hi;     //  0 (1.0)
        uint8_t     checksum;                       //  8 (1.0)
        char        oem_id[6];                      //  9 (1.0)
        uint8_t     revision;                       // 15 (1.0)
        uint32_t    rsdt_addr;                      // 16 (1.0)
        uint32_t    length;                         // 20 (2.0)
        uint32_t    xsdt_addr_lo, xsdt_addr_hi;     // 24 (2.0)
        uint8_t     extended_checksum;              // 32 (2.0)
        uint8_t     reserved[3];                    // 33 (2.0)

        [[nodiscard]] bool valid() const
        {
            // Use split loads to avoid unaligned accesses
            return (static_cast<uint64_t>(signature_hi) << 32 | signature_lo) == Signature::value ("RSD PTR ") &&
                   Checksum::additive (reinterpret_cast<uint8_t const *>(this), revision > 1 ? length : 20) == 0;
        }

    public:
        [[nodiscard]] bool parse (uint64_t &, size_t &) const;
};

static_assert (__is_standard_layout (Acpi_table_rsdp) && sizeof (Acpi_table_rsdp) == 36);
