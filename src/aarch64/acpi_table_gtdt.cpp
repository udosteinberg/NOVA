/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_table_gtdt.hpp"
#include "intid.hpp"
#include "timer.hpp"

void Acpi_table_gtdt::parse() const
{
    // Set timer PPI
    Timer::ppi_el2_p = Intid::to_ppi (tmr_el2_p_gsi);
    Timer::ppi_el1_v = Intid::to_ppi (tmr_el1_v_gsi);

    // Set timer trigger
    Timer::lvl_el2_p = !(tmr_el2_p_flg & Flags::TRIGGER_EDGE);
    Timer::lvl_el1_v = !(tmr_el1_v_flg & Flags::TRIGGER_EDGE);
}
