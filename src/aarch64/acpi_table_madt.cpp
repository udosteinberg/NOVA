/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "util.hpp"

void Acpi_table_madt::Controller_gicd::parse() const
{
    // Split loads avoid unaligned accesses
    auto const gicd { static_cast<uint64_t>(phys_gicd_hi) << 32 | phys_gicd_lo };

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICD:%#010lx", gicd);

    Gicd::phys = gicd;
}

void Acpi_table_madt::Controller_gicr::parse() const
{
    // Split loads avoid unaligned accesses
    auto const gicr { static_cast<uint64_t>(phys_gicr_hi) << 32 | phys_gicr_lo };

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICR:%#010lx", gicr);

    Gicr::assign (gicr);
}

void Acpi_table_madt::Controller_gicc::parse() const
{
    // The CPU is unusable
    if (!(flags & Flags::ENABLED))
        return;

    // The CPU uses the parking protocol, which is not (yet) supported
    if (park_pver)
        return;

    // Split loads avoid unaligned accesses
    auto const gicc { static_cast<uint64_t>(phys_gicc_hi) << 32 | phys_gicc_lo };
    auto const gich { static_cast<uint64_t>(phys_gich_hi) << 32 | phys_gich_lo };
    auto const gicr { static_cast<uint64_t>(phys_gicr_hi) << 32 | phys_gicr_lo };

    if (gicc)
        Gicc::phys = gicc;
    if (gich)
        Gich::phys = gich;

    // MPIDR format: Aff3[39:32] Aff2[23:16] Aff1[15:8] Aff0[7:0]
    auto const mpidr { static_cast<uint64_t>(val_mpidr_hi) << 32 | val_mpidr_lo };

    if (Psci::states && Psci::boot_cpu (Cpu::count, mpidr))
        Cpu::allocate (Cpu::count++, mpidr, gicr);
}

void Acpi_table_madt::Controller_gits::parse() const {}
void Acpi_table_madt::Controller_gmsi::parse() const {}

void Acpi_table_madt::parse() const
{
    auto const len { table.header.length };
    auto const ptr { reinterpret_cast<uintptr_t>(this) };

    if (EXPECT_FALSE (len < sizeof (*this)))
        return;

    // Set 32-bit GICC address and let GICC structure override it with a 64-bit address
    Gicc::phys = phys;

    for (auto p { ptr + sizeof (*this) }; p < ptr + len; ) {

        auto const c { reinterpret_cast<Controller const *>(p) };

        switch (c->type) {
            case Controller::GICD: static_cast<Controller_gicd const *>(c)->parse(); break;
            case Controller::GICR: static_cast<Controller_gicr const *>(c)->parse(); break;
            case Controller::GICC: static_cast<Controller_gicc const *>(c)->parse(); break;
            case Controller::GITS: static_cast<Controller_gits const *>(c)->parse(); break;
            case Controller::GMSI: static_cast<Controller_gmsi const *>(c)->parse(); break;
            default: break;
        }

        p += c->length;
    }
}
