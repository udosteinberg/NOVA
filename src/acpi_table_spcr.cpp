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

#include "acpi_table_spcr.hpp"
#include "stdio.hpp"

void Acpi_table_spcr::parse() const
{
    trace (TRACE_FIRM, "SPCR: Console %04x:%04x (%u:%#lx:%u:%u)", std::to_underlying (Debug::Type::SERIAL), uint16_t { subtype }, std::to_underlying (regs.asid), uint64_t { regs.addr }, uint8_t { regs.bits }, uint8_t { regs.accs });

    Console::bind (Debug::Type::SERIAL, Debug::Subtype { uint16_t { subtype } }, regs);
}
