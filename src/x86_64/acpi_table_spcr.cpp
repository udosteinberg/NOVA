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

#include "acpi_table_spcr.hpp"
#include "stdio.hpp"

void Acpi_table_spcr::parse() const
{
    Console::bind (&regs, uart_type);

    trace (TRACE_FIRM, "SPCR: Console %#x at %u:%#lx:%u", std::to_underlying (uart_type), std::to_underlying (regs.asid), regs.addr, regs.bits);
}
