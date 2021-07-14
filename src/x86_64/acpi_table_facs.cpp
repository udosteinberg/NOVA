/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_table_facs.hpp"
#include "stdio.hpp"

void Acpi_table_facs::parse() const
{
    trace (TRACE_FIRM, "FACS: Signature %#x Flags %#x Wake %#x/%#llx", hw_signature, flags, fw_waking_vector_32, fw_waking_vector_64);
}
