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
 * 5.2.24: Generic Timer Description Table (GTDT)
 */
class Acpi_table_gtdt final
{
    private:
        Acpi_table                  table;                      //  0   5.0+
        Unaligned_le<uint64_t>      ctrl_base;                  //  36  5.0+
        Unaligned_le<uint32_t>      flags;                      //  44  5.0+
        Unaligned_le<uint32_t>      el1_s_gsi, el1_s_flg;       //  48  5.0+
        Unaligned_le<uint32_t>      el1_p_gsi, el1_p_flg;       //  56  5.0+
        Unaligned_le<uint32_t>      el1_v_gsi, el1_v_flg;       //  64  5.0+
        Unaligned_le<uint32_t>      el2_p_gsi, el2_p_flg;       //  72  5.0+
        Unaligned_le<uint64_t>      read_base;                  //  80  5.1+
        Unaligned_le<uint32_t>      plt_cnt;                    //  88  5.1+
        Unaligned_le<uint32_t>      plt_off;                    //  92  5.1+
        Unaligned_le<uint32_t>      el2_v_gsi, el2_v_flg;       //  96  6.3+

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_gtdt) && alignof (Acpi_table_gtdt) == 1 && sizeof (Acpi_table_gtdt) == 104);
