/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "types.hpp"

/*
 * 5.2.5: Root System Description Pointer (RSDP)
 */
class Acpi_rsdp
{
    private:
        uint64  signature;                          //  0 (1.0)
        uint8   checksum;                           //  8 (1.0)
        char    oem_id[6];                          //  9 (1.0)
        uint8   revision;                           // 15 (1.0)
        uint32  rsdt_addr;                          // 16 (1.0)
        uint32  length;                             // 20 (2.0)
        uint64  xsdt_addr;                          // 24 (2.0)
        uint8   extended_checksum;                  // 32 (2.0)
        uint8   reserved[3];                        // 33 (2.0)

        inline bool valid() const
        {
            if (signature != 0x2052545020445352)
                return false;

            auto len = revision > 1 ? length : 20;

            uint8 check = 0;
            for (auto ptr = reinterpret_cast<uint8 const *>(this);
                      ptr < reinterpret_cast<uint8 const *>(this) + len;
                      check = static_cast<uint8>(check + *ptr++)) ;

            return !check;
        }

    public:
        bool parse (uint64 &, size_t &) const;
};
