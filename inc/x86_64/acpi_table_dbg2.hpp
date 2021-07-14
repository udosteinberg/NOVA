/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "debug.hpp"

#pragma pack(1)

/*
 * Debug Port Table 2 (DBG2)
 */
class Acpi_table_dbg2 final
{
    private:
        Acpi_table          table;                  //  0
        uint32              info_off;               // 36
        uint32              info_cnt;               // 40

        struct Info
        {
            uint8           revision;               //  0
            uint16          length;                 //  1
            uint8           regs_cnt;               //  3
            uint16          nstr_len;               //  4
            uint16          nstr_off;               //  6
            uint16          data_len;               //  8
            uint16          data_off;               // 10
            Debug::Type     type;                   // 12
            Debug::Subtype  subtype;                // 14
            uint16          reserved;               // 16
            uint16          regs_off;               // 18
            uint16          size_off;               // 20
        };

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_dbg2) && sizeof (Acpi_table_dbg2) == 44);

#pragma pack()
