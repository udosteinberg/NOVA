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
#include "cpu.hpp"
#include "hazards.hpp"
#include "lowlevel.hpp"
#include "stdio.hpp"
#include "uefi.hpp"

bool Acpi::init()
{
    auto &rsdp = reinterpret_cast<Uefi *>(Kmem::sym_to_virt (&Uefi::uefi))->rsdp;

    if (!rsdp && !(rsdp = rsdp_find()))
        return false;

    uint64 rsdt;
    size_t size;

    if (static_cast<Acpi_table_rsdp *>(Hptp::map (rsdp))->parse (rsdt, size) == false)
        return false;

    if (rsdt)
        static_cast<Acpi_table_rsdt *>(Hptp::map (rsdt))->parse (rsdt, size);
    if (fadt)
        static_cast<Acpi_table_fadt *>(Hptp::map (fadt))->parse();
    if (facs)
        static_cast<Acpi_table_facs *>(Hptp::map (facs))->parse();

    parse_tables();

    return true;
}

void Acpi::sleep()
{
    Cpu::hazard &= ~HZD_SLEEP;

    auto v = get_state();
    auto s = v & BIT_RANGE (2, 0);

    if (Cpu::bsp) {

        for (Cpu::online--; Cpu::online; pause()) ;

        if (BIT_RANGE (5, 1) & BIT (s))
            trace (TRACE_FIRM, "ACPI: Entering S%u (%u:%u)", s, v >> 3 & BIT_RANGE (2, 0), v >> 6 & BIT_RANGE (2, 0));
        else
            trace (TRACE_FIRM, "ACPI: Resetting");

        if (BIT_RANGE (3, 1) & BIT (s))
            Cache::data_clean();

        if (BIT_RANGE (5, 1) & BIT (s))
            Acpi_fixed::sleep (v >> 3 & BIT_RANGE (2, 0), v >> 6 & BIT_RANGE (2, 0));
        else
            Acpi_fixed::reset();

    } else {

        if (BIT_RANGE (3, 1) & BIT (s))
            Cache::data_clean();

        Cpu::online--;
    }

    // WAK_STS is W1C for software, so only hardware can set it
    Acpi_fixed::wake_chk();

    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    if (Cpu::bsp)
        clr_state();
}
