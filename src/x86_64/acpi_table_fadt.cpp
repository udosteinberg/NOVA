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

#include "acpi.hpp"
#include "acpi_fixed.hpp"
#include "stdio.hpp"

void Acpi_table_fadt::parse() const
{
    trace (TRACE_FIRM, "ACPI: Version %u.%u Profile %u (%s)", revision, length > 131 ? minor_version : 0, pm_profile, fflg & HW_REDUCED_ACPI ? "Reduced" : "Fixed");

    Acpi::facs = facs64 ? facs64 : facs32;
    Acpi::fflg = fflg;
    Acpi::bflg = bflg_x86;

    // XXX: Use x_pm blocks if they exist

    if (pm1a_evt_blk) {
        Acpi_fixed::pm1a_sts.init (Acpi_gas::Asid::IO, pm1_evt_len >> 1, pm1a_evt_blk);
        Acpi_fixed::pm1a_ena.init (Acpi_gas::Asid::IO, pm1_evt_len >> 1, pm1a_evt_blk + (pm1_evt_len >> 1));
    }
    if (pm1b_evt_blk) {
        Acpi_fixed::pm1b_sts.init (Acpi_gas::Asid::IO, pm1_evt_len >> 1, pm1b_evt_blk);
        Acpi_fixed::pm1b_ena.init (Acpi_gas::Asid::IO, pm1_evt_len >> 1, pm1b_evt_blk + (pm1_evt_len >> 1));
    }

    if (pm1a_cnt_blk)
        Acpi_fixed::pm1a_cnt.init (Acpi_gas::Asid::IO, pm1_cnt_len, pm1a_cnt_blk);

    if (pm1b_cnt_blk)
        Acpi_fixed::pm1b_cnt.init (Acpi_gas::Asid::IO, pm1_cnt_len, pm1b_cnt_blk);

    if (pm2_cnt_blk)
        Acpi_fixed::pm2_cnt.init (Acpi_gas::Asid::IO, pm2_cnt_len, pm2_cnt_blk);

    if (pm_tmr_blk)
        Acpi_fixed::pm_tmr.init (Acpi_gas::Asid::IO, pm_tmr_len, pm_tmr_blk);

    if (length >= 129) {
        Acpi_fixed::reset_reg = reset_reg;
        Acpi_fixed::reset_val = reset_value;
    }

    Acpi_fixed::enable (smi_cmd, acpi_enable);
}
