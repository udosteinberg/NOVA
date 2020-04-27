/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "acpi_table_tpm2.hpp"
#include "stdio.hpp"

void Acpi_table_tpm2::parse() const
{
    auto const len { table.header.length };

    if (EXPECT_FALSE (len < sizeof (*this)))
        return;

    trace (TRACE_FIRM, "TPM2: TPM at %#lx (%u)", tpm_base, start_method);
}
