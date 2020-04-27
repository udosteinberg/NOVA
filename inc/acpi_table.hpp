/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "macros.hpp"
#include "types.hpp"

/*
 * 5.2.6: System Description Table Header
 */
class Acpi_table
{
    protected:
        uint32      signature;                      //  0 (1.0)
        uint32      length;                         //  4 (1.0)
        uint8       revision;                       //  8 (1.0)
        uint8       checksum;                       //  9 (1.0)
        char        oem_id[6];                      // 10 (1.0)
        char        oem_table_id[8];                // 16 (1.0)
        uint32      oem_revision;                   // 24 (1.0)
        char        creator_id[4];                  // 28 (1.0)
        uint32      creator_revision;               // 32 (1.0)

        inline bool valid() const
        {
            uint8 check = 0;
            for (auto ptr = reinterpret_cast<uint8 const *>(this);
                      ptr < reinterpret_cast<uint8 const *>(this) + length;
                      check = static_cast<uint8>(check + *ptr++)) ;

            return !check;
        }

    public:
        static inline constexpr auto sig (char const (&x)[5])
        {
            return x[3] << 24 | x[2] << 16 | x[1] << 8 | x[0];
        }

        bool validate (uint64) const;
};
