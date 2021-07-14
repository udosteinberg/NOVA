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

#include "acpi_table_spcr.hpp"
#include "stdio.hpp"

void Acpi_table_spcr::parse() const
{
    if (EXPECT_FALSE (table.header.length < sizeof (*this)))
        return;

    trace (TRACE_FIRM, "SPCR: Console %04x:%04x (%u:%#lx:%u:%u)", std::to_underlying (Debug::Type::SERIAL), std::to_underlying (subtype), std::to_underlying (regs.asid), regs.addr, regs.bits, regs.size);

    Console::bind (Debug::Type::SERIAL, subtype, regs);
}
