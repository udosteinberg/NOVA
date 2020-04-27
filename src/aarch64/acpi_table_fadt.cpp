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
#include "stdio.hpp"

void Acpi_table_fadt::parse() const
{
    Acpi::facs = facs64 ? facs64 : facs32;
    Acpi::fflg = fflg;

    trace (TRACE_FIRM, "ACPI: Version %u.%u Profile %u Features %#x Boot %#x", uint8_t { table.revision }, minor_version & BIT_RANGE (3, 0), uint8_t { pm_profile }, uint32_t { fflg }, uint16_t { bflg_arm });

    if (bflg_arm & BIT (0))
        Psci::init();
}
