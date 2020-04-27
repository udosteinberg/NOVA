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

#include "acpi_table_madt.hpp"
#include "gicc.hpp"
#include "gicd.hpp"
#include "gich.hpp"
#include "gicr.hpp"
#include "psci.hpp"
#include "stdio.hpp"

void Acpi_table_madt::Controller_gicd::parse() const
{
    uint64_t const gicd { phys_gicd };

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICD:%#010lx", gicd);

    Gicd::phys = gicd;
}

void Acpi_table_madt::Controller_gicr::parse() const
{
    uint64_t const gicr { phys_gicr };

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICR:%#010lx", gicr);

    Gicr::assign (gicr);
}

void Acpi_table_madt::Controller_gicc::parse() const
{
    // The CPU is unusable
    if (!(flags & BIT (0)))
        return;

    // The CPU uses the parking protocol, which is not (yet) supported
    if (park_pver)
        return;

    if (phys_gicc)
        Gicc::phys = phys_gicc;
    if (phys_gich)
        Gich::phys = phys_gich;

    // MPIDR format: Aff3[39:32] Aff2[23:16] Aff1[15:8] Aff0[7:0]
    auto const mpidr { val_mpidr };

    if (Psci::states && Psci::boot_cpu (Cpu::count, mpidr))
        Cpu::allocate (Cpu::count++, mpidr, phys_gicr);
}

void Acpi_table_madt::Controller_gits::parse() const {}
void Acpi_table_madt::Controller_gmsi::parse() const {}

void Acpi_table_madt::parse() const
{
    // Set 32-bit GICC address and let GICC structure override it with a 64-bit address
    Gicc::phys = phys;

    for (auto ptr { reinterpret_cast<uintptr_t>(this + 1) }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ) {

        auto const c { reinterpret_cast<Controller const *>(ptr) };

        switch (c->type()) {
            case Controller::Type::GICD: static_cast<Controller_gicd const *>(c)->parse(); break;
            case Controller::Type::GICR: static_cast<Controller_gicr const *>(c)->parse(); break;
            case Controller::Type::GICC: static_cast<Controller_gicc const *>(c)->parse(); break;
            case Controller::Type::GITS: static_cast<Controller_gits const *>(c)->parse(); break;
            case Controller::Type::GMSI: static_cast<Controller_gmsi const *>(c)->parse(); break;
            default: break;
        }

        ptr += c->length;
    }
}
