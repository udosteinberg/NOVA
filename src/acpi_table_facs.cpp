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

#include "acpi_table_facs.hpp"
#include "stdio.hpp"

void Acpi_table_facs::parse() const
{
    if (header.length < sizeof (*this)) [[unlikely]]
        return;

    trace (TRACE_FIRM, "FACS: Hardware %#x Flags %#x Wake %#x/%#lx", uint32_t { hardware }, uint32_t { flags }, uint32_t { wake32 }, uint64_t { wake64 });
}
