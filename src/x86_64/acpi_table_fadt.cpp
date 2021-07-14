/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "acpi.hpp"
#include "cmdline.hpp"
#include "space_pio.hpp"
#include "stdio.hpp"

void Acpi_table_fadt::parse() const
{
    Acpi::facs = facs64 ? facs64 : facs32;
    Acpi::fflg = fflg;

    trace (TRACE_FIRM, "ACPI: Version %u.%u Profile %u Features %#x Boot %#x", uint8_t { table.revision }, minor_version & BIT_RANGE (3, 0), uint8_t { pm_profile }, uint32_t { fflg }, uint16_t { bflg_x86 });

    // Check for RESET register support
    if (fflg & BIT (10)) {
        Acpi_fixed::rst_reg = reset_reg;
        Acpi_fixed::rst_val = reset_val;
    }

    // Check for HW-reduced ACPI
    if (fflg & BIT (20)) {
        Acpi_fixed::slp_cnt = sleep_cnt;
        Acpi_fixed::slp_sts = sleep_sts;
        return;
    }

    // Functionality below does not exist on HW-reduced platforms

    // GPE register blocks can have more bits than what can be encoded in Acpi_gas
    Acpi_fixed::gpe0_len = gpe0_blk_len / 2;
    Acpi_fixed::gpe1_len = gpe1_blk_len / 2;

    struct
    {
        Acpi_gas &          reg;
        Acpi_gas const &    x_blk;
        uint32_t const      blk;
        uint8_t const       len, cnt, idx;
        bool const          res;
    } regs[] {
        { Acpi_fixed::gpe0_sts, x_gpe0_blk,     gpe0_blk,     0,            2, 0, false },  // optional
        { Acpi_fixed::gpe0_ena, x_gpe0_blk,     gpe0_blk,     0,            2, 1, false },  // optional
        { Acpi_fixed::gpe1_sts, x_gpe1_blk,     gpe1_blk,     0,            2, 0, false },  // optional
        { Acpi_fixed::gpe1_ena, x_gpe1_blk,     gpe1_blk,     0,            2, 1, false },  // optional
        { Acpi_fixed::pm1a_sts, x_pm1a_evt_blk, pm1a_evt_blk, pm1_evt_len,  2, 0, false },  // required
        { Acpi_fixed::pm1a_ena, x_pm1a_evt_blk, pm1a_evt_blk, pm1_evt_len,  2, 1, false },  // required
        { Acpi_fixed::pm1b_sts, x_pm1b_evt_blk, pm1b_evt_blk, pm1_evt_len,  2, 0, false },  // optional
        { Acpi_fixed::pm1b_ena, x_pm1b_evt_blk, pm1b_evt_blk, pm1_evt_len,  2, 1, false },  // optional
        { Acpi_fixed::pm1a_cnt, x_pm1a_cnt_blk, pm1a_cnt_blk, pm1_cnt_len,  1, 0, true  },  // required
        { Acpi_fixed::pm1b_cnt, x_pm1b_cnt_blk, pm1b_cnt_blk, pm1_cnt_len,  1, 0, true  },  // optional
        { Acpi_fixed::pm2_cnt,  x_pm2_cnt_blk,  pm2_cnt_blk,  pm2_cnt_len,  1, 0, true  },  // optional
        { Acpi_fixed::pm_tmr,   x_pm_tmr_blk,   pm_tmr_blk,   pm_tmr_len,   1, 0, false },  // optional
    };

    for (unsigned i { 0 }; i < sizeof (regs) / sizeof (*regs); i++) {

        regs[i].reg = Acpi_gas { regs[i].x_blk, regs[i].blk, regs[i].len, regs[i].cnt, regs[i].idx };

        if (regs[i].res)
            switch (regs[i].reg.asid) {
             // case Acpi_gas::Asid::MEM: Space_hst::access_ctrl (regs[i].reg.addr, regs[i].reg.bits / 8, Paging::NONE); break;
                case Acpi_gas::Asid::PIO: Space_pio::access_ctrl (regs[i].reg.addr, regs[i].reg.bits / 8, Paging::NONE); break;
                default: break;
            }
    }

    if (smi_cmd) {

        Acpi_fixed::enable (static_cast<port_t>(smi_cmd), acpi_enable, pstate_cnt, cstate_cnt);

        if (!Cmdline::insecure)
            Space_pio::access_ctrl (smi_cmd, 1, Paging::NONE);
    }
}
