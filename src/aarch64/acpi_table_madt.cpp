/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "gits.hpp"
#include "smc_psci.hpp"
#include "stdio.hpp"

void Acpi_table_madt::Controller_gicd::parse() const
{
    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICD:%#010lx", uint64_t { phys_gicd });

    Gicd::phys = phys_gicd;
}

void Acpi_table_madt::Controller_gicr::parse() const
{
    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICR:%#010lx (%#x)", uint64_t { phys_gicr }, uint32_t { size_gicr });

    // GICR is not coherent
    if (flags & BIT (0)) [[unlikely]]
        Gicr::coherent = false;

    if (!Gicr::enumerate (phys_gicr, size_gicr)) [[unlikely]]
        panic ("GICR enumeration failed");
}

void Acpi_table_madt::Controller_gicc::parse() const
{
    // CPU is unusable
    if (!(flags & BIT (0))) [[unlikely]]
        return;

    // GICR is not coherent
    if (flags & BIT (4)) [[unlikely]]
        Gicr::coherent = false;

    // The CPU uses the parking protocol, which is deprecated since MADT version 7
    if (park_pver) [[unlikely]]
        return;

    if (phys_gicc) [[likely]]
        Gicc::phys = phys_gicc;

    if (phys_gich) [[likely]]
        Gich::phys = phys_gich;

    // MPIDR format: Aff3[39:32] Aff2[23:16] Aff1[15:8] Aff0[7:0]
    auto const mpidr { val_mpidr };

    if (Smc_psci::states && Smc_psci::boot_cpu (Cpu::count, mpidr)) [[likely]]
        Cpu::allocate (Cpu::count++, mpidr, phys_gicr);
}

void Acpi_table_madt::Controller_gits::parse() const
{
    // GITS is not coherent
    if (flags & BIT (0)) [[unlikely]]
        Gits::coherent = false;

    auto const gits { Gits::setup (phys_gits, id) };

    if (!gits) [[unlikely]]
        panic ("GITS allocation failed");
}

void Acpi_table_madt::parse_entry (Controller::Type t) const
{
    Controller const *c;

    for (auto ptr { reinterpret_cast<uintptr_t>(this + 1) }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ptr += c->length) {

        c = reinterpret_cast<Controller const *>(ptr);

        if (c->type() != t) [[likely]]
            continue;

        switch (c->type()) {
            case Controller::Type::GICD: static_cast<Controller_gicd const *>(c)->parse(); break;
            case Controller::Type::GICR: static_cast<Controller_gicr const *>(c)->parse(); break;
            case Controller::Type::GICC: static_cast<Controller_gicc const *>(c)->parse(); break;
            case Controller::Type::GITS: static_cast<Controller_gits const *>(c)->parse(); break;
            default: break;
        }
    }
}

void Acpi_table_madt::parse() const
{
    // Set 32-bit GICC address and let GICC structure override it with a 64-bit address
    Gicc::phys = phys;

    // Parsing order matters: GICR/GITS need Cpu::count, which GICC provides
    parse_entry (Controller::Type::GICC);
    parse_entry (Controller::Type::GICR);
    parse_entry (Controller::Type::GICD);
    parse_entry (Controller::Type::GITS);
}
