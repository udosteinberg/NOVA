/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_table.hpp"

#pragma pack(1)

/*
 * Debug Port Table 2 (DBG2)
 */
class Acpi_table_dbg2 : private Acpi_table
{
    friend class Acpi_table_spcr;

    private:
        uint32      info_off;                       // 36
        uint32      info_cnt;                       // 40

        struct Info
        {
            enum class Type : uint16
            {
                SERIAL          = 0x8000,
                FIREWIRE        = 0x8001,
                USB             = 0x8002,
                NET             = 0x8003,
            };

            enum class Subtype : uint16
            {
                SERIAL_PL011    = 0x0003,
                SERIAL_NS16550  = 0x0012,
            };

            uint8   revision;                       //  0
            uint16  length;                         //  1
            uint8   regs_cnt;                       //  3
            uint16  nstr_len;                       //  4
            uint16  nstr_off;                       //  6
            uint16  data_len;                       //  8
            uint16  data_off;                       // 10
            Type    type;                           // 12
            Subtype subtype;                        // 14
            uint16  reserved;                       // 16
            uint16  regs_off;                       // 18
            uint16  size_off;                       // 20
        };

    public:
        void parse() const;
};

#pragma pack()
