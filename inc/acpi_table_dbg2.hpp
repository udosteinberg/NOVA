/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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
 * Debug Port Table 2 (DBG2)
 */
class Acpi_table_dbg2 final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      info_off;                   // 36
        Unaligned_le<uint32_t>      info_cnt;                   // 40

        struct Info
        {
            Unaligned_le<uint8_t>   revision;                   // 0
            Unaligned_le<uint16_t>  length;                     // 1
            Unaligned_le<uint8_t>   regs_cnt;                   // 3
            Unaligned_le<uint16_t>  nstr_len;                   // 4
            Unaligned_le<uint16_t>  nstr_off;                   // 6
            Unaligned_le<uint16_t>  data_len;                   // 8
            Unaligned_le<uint16_t>  data_off;                   // 10
            Unaligned_le<uint16_t>  type;                       // 12
            Unaligned_le<uint16_t>  subtype;                    // 14
            Unaligned_le<uint16_t>  reserved;                   // 16
            Unaligned_le<uint16_t>  regs_off;                   // 18
            Unaligned_le<uint16_t>  size_off;                   // 20
        };

        static_assert (__is_standard_layout (Info) && alignof (Info) == 1 && sizeof (Info) == 22);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_dbg2) && alignof (Acpi_table_dbg2) == 1 && sizeof (Acpi_table_dbg2) == 44);
