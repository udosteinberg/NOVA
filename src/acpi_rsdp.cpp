/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#include "acpi.h"
#include "acpi_rsdp.h"
#include "hpt.h"

Acpi_rsdp *Acpi_rsdp::find (mword start, unsigned len)
{
    for (mword addr = start; addr < start + len; addr += 16) {
        Acpi_rsdp *rsdp = reinterpret_cast<Acpi_rsdp *>(addr);
        if (rsdp->good_signature() && rsdp->good_checksum())
            return rsdp;
    }

    return 0;
}

void Acpi_rsdp::parse()
{
    Acpi_rsdp *rsdp;

    mword map = reinterpret_cast<mword>(Hpt::remap (0));

    if (!(rsdp = Acpi_rsdp::find (map + (*reinterpret_cast<uint16 *>(map + 0x40e) << 4), 0x400)) &&
        !(rsdp = Acpi_rsdp::find (map + 0xe0000, 0x20000)))
        return;

    Acpi::rsdt = rsdp->rsdt_addr;

    if (rsdp->revision > 1 && rsdp->good_checksum (rsdp->length))
        Acpi::xsdt = static_cast<Paddr>(rsdp->xsdt_addr);
}
