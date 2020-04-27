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

/*
 * 5.2.24: Generic Timer Description Table (GTDT)
 */
class Acpi_table_gtdt : private Acpi_table
{
    private:
        enum Flags : uint32
        {
            TRIGGER_EDGE    = BIT  (0),
            POLARITY_LOW    = BIT  (1),
            ALWAYS_ON       = BIT  (2),
        };

        uint32      ctrl_base_lo, ctrl_base_hi;     //  36 (5.0)
        uint32      global_flags;                   //  44 (5.0)
        uint32      tmr_el1_s_gsi;                  //  48 (5.0)
        Flags       tmr_el1_s_flg;                  //  52 (5.0)
        uint32      tmr_el1_p_gsi;                  //  56 (5.0)
        Flags       tmr_el1_p_flg;                  //  60 (5.0)
        uint32      tmr_el1_v_gsi;                  //  64 (5.0)
        Flags       tmr_el1_v_flg;                  //  68 (5.0)
        uint32      tmr_el2_p_gsi;                  //  72 (5.0)
        Flags       tmr_el2_p_flg;                  //  76 (5.0)
        uint32      read_base_lo, read_base_hi;     //  80 (5.1)
        uint32      tmr_plt_count;                  //  88 (5.1)
        uint32      tmr_plt_offset;                 //  92 (5.1)
        uint32      tmr_el2_v_gsi;                  //  96 (6.3)
        Flags       tmr_el2_v_flg;                  // 100 (6.3)

    public:
        void parse() const;
};
