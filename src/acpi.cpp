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
#include "cpu.hpp"
#include "lowlevel.hpp"
#include "stdio.hpp"
#include "timer.hpp"
#include "uefi.hpp"

bool Acpi::init()
{
    if (!resume) {

        auto &rsdp { Uefi::info.rsdp };

        if (!rsdp && !(rsdp = rsdp_find()))
            return false;

        void const *ptr { Hptp::map_tmp (rsdp, sizeof (Acpi_table_rsdp), Paging::R, Memattr::ram(), 0) };
        if (!ptr || !static_cast<Acpi_table_rsdp const *>(ptr)->parse())
            return false;

        if ((ptr = Acpi_table::map (xsdt, 0))) [[likely]]
            static_cast<Acpi_table_xsdt const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (fadt, 0))) [[likely]]
            static_cast<Acpi_table_fadt const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (facs, 0))) [[likely]]
            static_cast<Acpi_table_facs const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (spcr, 0))) [[likely]]
            static_cast<Acpi_table_spcr const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (dbg2, 0))) [[likely]]
            static_cast<Acpi_table_dbg2 const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (madt, 0))) [[likely]]
            static_cast<Acpi_table_madt const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (mcfg, 0))) [[likely]]
            static_cast<Acpi_table_mcfg const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (srat, 0))) [[likely]]
            static_cast<Acpi_table_srat const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (hest, 0))) [[likely]]
            static_cast<Acpi_table_hest const *>(ptr)->parse();
        if ((ptr = Acpi_table::map (tpm2, 0))) [[likely]]
            static_cast<Acpi_table_tpm2 const *>(ptr)->parse();

        parse_tables();

        wake_prepare();
    }

    clr_transition();

    return true;
}

void Acpi::fini (Acpi_fixed::Transition s)
{
    auto b = BIT (s.state());

    if (Cpu::bsp) {

        for (Cpu::online--; Cpu::online; pause()) ;

        Acpi_fixed::offline_wait();

        if (BIT_RANGE (5, 1) & b)
            trace (TRACE_FIRM, "ACPI: Entering S%u", s.state());
        else
            trace (TRACE_FIRM, "ACPI: Resetting");

        if (BIT_RANGE (3, 2) & b)
            wake_prepare();

        Acpi::resume = Timer::time();

        if (BIT_RANGE (3, 1) & b)
            Cache::data_clean();

        bool ok = BIT_RANGE (5, 1) & b ? Acpi_fixed::sleep (s) : Acpi_fixed::reset();

        if (!ok)
            trace (TRACE_FIRM, "ACPI: Transition failed");

    } else {

        if (BIT_RANGE (3, 1) & b)
            Cache::data_clean();

        Cpu::online--;

        Acpi_fixed::offline_core();
    }

    // WAK_STS is W1C for software, so only hardware can set it
    Acpi_fixed::wake_chk();

    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    if (Cpu::bsp)
        clr_transition();
}
