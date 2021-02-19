/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "acpi_table.hpp"
#include "stdio.hpp"

bool Acpi_table::good_checksum (Paddr addr) const
{
    uint8 check = 0;

    for (uint8 const *ptr = reinterpret_cast<uint8 const *>(this);
                      ptr < reinterpret_cast<uint8 const *>(this) + length;
                      check = static_cast<uint8>(check + *ptr++)) ;

    trace (TRACE_FIRM, "%.4s:%#010llx REV:%2d TBL:%8.8s OEM:%6.6s LEN:%5u (%s %#04x)",
           reinterpret_cast<char const *>(&signature),
           static_cast<uint64>(addr),
           revision,
           oem_table_id,
           oem_id,
           length,
           check ? "bad" : "ok",
           static_cast<unsigned int>(checksum));

    return !check;
}
