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

bool Acpi_table_madt::Controller_gicd::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICD:%#010lx", uint64_t { phys_gicd });

    Gicd::phys = phys_gicd;

    return true;
}

bool Acpi_table_madt::Controller_gicr::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICR:%#010lx (%#x)", uint64_t { phys_gicr }, uint32_t { size_gicr });

    // GICR is noncoherent
    if (flags & BIT (0)) [[unlikely]]
        Gicr::noncoherent = true;

    if (!Gicr::enumerate (phys_gicr, size_gicr)) [[unlikely]]
        panic ("GICR enumeration failed");

    return true;
}

bool Acpi_table_madt::Controller_gicc::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    // CPU is unusable
    if (!(flags & BIT (0))) [[unlikely]]
        return true;

    // GICR is noncoherent
    if (flags & BIT (4)) [[unlikely]]
        Gicr::noncoherent = true;

    // The CPU uses the parking protocol, which is deprecated since MADT version 7
    if (park_pver) [[unlikely]]
        return true;

    if (phys_gicc) [[likely]]
        Gicc::phys = phys_gicc;

    if (phys_gich) [[likely]]
        Gich::phys = phys_gich;

    // MPIDR format: Aff3[39:32] Aff2[23:16] Aff1[15:8] Aff0[7:0]
    auto const mpidr { val_mpidr };

    if (Smc_psci::states && Smc_psci::boot_cpu (Cpu::count, mpidr)) [[likely]]
        Cpu::allocate (Cpu::count++, mpidr, phys_gicr);

    return true;
}

bool Acpi_table_madt::Controller_gits::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    // GITS is noncoherent
    if (flags & BIT (0)) [[unlikely]]
        Gits::noncoherent = true;

    auto const gits { Gits::setup (phys_gits, id) };

    if (!gits) [[unlikely]]
        panic ("GITS allocation failed");

    return true;
}

void Acpi_table_madt::parse_entry (Controller::Type const t, uintptr_t ptr, uintptr_t const end, bool &ret)
{
    using list_t = Controller;

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };
        auto const l { s->len };

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        // Skip entries that don't match the requested type
        if (s->type() != t) [[likely]]
            continue;

        switch (t) {
            case Controller::Type::GICD: ret &= std::start_lifetime_as<Controller_gicd const> (s)->parse(); break;
            case Controller::Type::GICR: ret &= std::start_lifetime_as<Controller_gicr const> (s)->parse(); break;
            case Controller::Type::GICC: ret &= std::start_lifetime_as<Controller_gicc const> (s)->parse(); break;
            case Controller::Type::GITS: ret &= std::start_lifetime_as<Controller_gits const> (s)->parse(); break;
            default: break;
        }
    }

    ret &= end == ptr;
}

bool Acpi_table_madt::parse() const
{
    auto const ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    // Set 32-bit GICC address and let GICC structure override it with a 64-bit address
    Gicc::phys = phys;

    bool ret { true };

    // Parsing order matters: GICR/GITS need Cpu::count, which GICC provides
    parse_entry (Controller::Type::GICC, ptr, end, ret);
    parse_entry (Controller::Type::GICR, ptr, end, ret);
    parse_entry (Controller::Type::GICD, ptr, end, ret);
    parse_entry (Controller::Type::GITS, ptr, end, ret);

    return ret;
}
