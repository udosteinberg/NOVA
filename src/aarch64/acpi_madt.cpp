/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "acpi_madt.hpp"
#include "gicc.hpp"
#include "gicd.hpp"
#include "gich.hpp"
#include "gicr.hpp"
#include "psci.hpp"
#include "stdio.hpp"
#include "util.hpp"

void Acpi_table_madt::Controller_gicd::parse() const
{
    // Use split loads to avoid unaligned accesses
    Gicd::phys = static_cast<uint64>(phys_gicd_hi) << 32 | phys_gicd_lo;

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICD:%#010llx", Gicd::phys);
}

void Acpi_table_madt::Controller_gicr::parse() const
{
    // Use split loads to avoid unaligned accesses
    Gicr::phys = static_cast<uint64>(phys_gicr_hi) << 32 | phys_gicr_lo;

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: GICR:%#010llx", Gicr::phys);
}

void Acpi_table_madt::Controller_gicc::parse() const
{
    // The CPU is unusable
    if (!(flags & Flags::ENABLED))
        return;

    // The CPU uses the parking protocol, which is not (yet) supported
    if (park_pver)
        return;

    // Use split loads to avoid unaligned accesses
    auto gicc = static_cast<uint64>(phys_gicc_hi) << 32 | phys_gicc_lo;
    auto gich = static_cast<uint64>(phys_gich_hi) << 32 | phys_gich_lo;
    auto gicr = static_cast<uint64>(phys_gicr_hi) << 32 | phys_gicr_lo;

    if (gicc)
        Gicc::phys = gicc;
    if (gich)
        Gich::phys = gich;
    if (gicr)
        Gicr::phys = Gicr::phys ? min (Gicr::phys, gicr) : gicr;

    trace (TRACE_FIRM | TRACE_PARSE, "MADT: CPU:%u GICC:%#010llx GICH:%#010llx", cpu, Gicc::phys, Gich::phys);

    // MPIDR format: Aff3[39:32] Aff2[23:16] Aff1[15:8] Aff0[7:0]
    auto mpidr = static_cast<uint64>(val_mpidr_hi) << 32 | val_mpidr_lo;

    // FIXME: Check FADT for PSCI compliance
    if (Cpu::online == 0 || Psci::boot_cpu (Cpu::online, mpidr))
        Cpu::online++;
}

void Acpi_table_madt::Controller_gits::parse() const {}
void Acpi_table_madt::Controller_gmsi::parse() const {}

void Acpi_table_madt::parse() const
{
    // Set the generic MMIO address first and let the GICC structure override it
    Gicc::phys = phys;

    for (auto d = &data; d < reinterpret_cast<uint8 const *>(this) + length; ) {

        auto c = reinterpret_cast<Controller const *>(d);

        switch (c->type) {
            case Controller::GICD: static_cast<Controller_gicd const *>(c)->parse(); break;
            case Controller::GICR: static_cast<Controller_gicr const *>(c)->parse(); break;
            case Controller::GICC: static_cast<Controller_gicc const *>(c)->parse(); break;
            case Controller::GITS: static_cast<Controller_gits const *>(c)->parse(); break;
            case Controller::GMSI: static_cast<Controller_gmsi const *>(c)->parse(); break;
        }

        d += c->length;
    }
}
