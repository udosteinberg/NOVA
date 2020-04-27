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
#include "stdio.hpp"

void Acpi_table_fadt::parse() const
{
    // At least ACPI 2.0 is required
    if (EXPECT_FALSE (table.header.length < 244))
        return;

    Acpi::facs = facs64 ? facs64 : facs32;
    Acpi::fflg = fflg;

    trace (TRACE_FIRM, "ACPI: Version %u.%u Profile:%u Feature:%#x Boot:%#x", table.revision, minor_version & BIT_RANGE (3, 0), pm_profile, fflg, bflg_arm);

    if (bflg_arm & Boot_arm::PSCI_COMPLIANT)
        Psci::init();
}
