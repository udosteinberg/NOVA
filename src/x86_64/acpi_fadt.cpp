/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "acpi_fadt.hpp"
#include "io.hpp"
#include "x86.hpp"

void Acpi_table_fadt::parse() const
{
    Acpi::irq     = sci_irq;
    Acpi::feature = flags;

    // XXX: Use x_pm blocks if they exist

    if (pm1a_evt_blk) {
        Acpi::pm1a_sts.init (Acpi_gas::IO, pm1_evt_len >> 1, pm1a_evt_blk);
        Acpi::pm1a_ena.init (Acpi_gas::IO, pm1_evt_len >> 1, pm1a_evt_blk + (pm1_evt_len >> 1));
    }
    if (pm1b_evt_blk) {
        Acpi::pm1b_sts.init (Acpi_gas::IO, pm1_evt_len >> 1, pm1b_evt_blk);
        Acpi::pm1b_ena.init (Acpi_gas::IO, pm1_evt_len >> 1, pm1b_evt_blk + (pm1_evt_len >> 1));
    }

    if (pm1a_cnt_blk)
        Acpi::pm1a_cnt.init (Acpi_gas::IO, pm1_cnt_len, pm1a_cnt_blk);

    if (pm1b_cnt_blk)
        Acpi::pm1b_cnt.init (Acpi_gas::IO, pm1_cnt_len, pm1b_cnt_blk);

    if (pm2_cnt_blk)
        Acpi::pm2_cnt.init (Acpi_gas::IO, pm2_cnt_len, pm2_cnt_blk);

    if (pm_tmr_blk)
        Acpi::pm_tmr.init (Acpi_gas::IO, pm_tmr_len, pm_tmr_blk);

    if (length >= 129) {
        Acpi::reset_reg = reset_reg;
        Acpi::reset_val = reset_value;
    }

    if (smi_cmd && acpi_enable) {
        Io::out (smi_cmd, acpi_enable);
        while (!(Acpi::read (Acpi::PM1_CNT) & Acpi::PM1_CNT_SCI_EN))
            pause();
    }
}
