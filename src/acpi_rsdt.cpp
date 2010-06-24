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
#include "acpi_rsdt.h"
#include "ptab.h"

void Acpi_table_rsdt::parse (Paddr addr, size_t size) const
{
    if (!good_checksum (addr))
        return;

    unsigned count = entries (size);

    Paddr table[count];
    for (unsigned i = 0; i < count; i++)
        table[i] = static_cast<Paddr>(size == sizeof (*xsdt) ? xsdt[i] : rsdt[i]);

    for (unsigned i = 0; i < count; i++) {

        Acpi_table *acpi = static_cast<Acpi_table *>(Ptab::master()->remap (table[i]));

        if (!acpi->good_checksum (table[i]))
            continue;
        else if (acpi->signature == SIG ('A','P','I','C'))
            Acpi::madt = table[i];
        else if (acpi->signature == SIG ('D','M','A','R'))
            Acpi::dmar = table[i];
        else if (acpi->signature == SIG ('F','A','C','P'))
            Acpi::fadt = table[i];
        else if (acpi->signature == SIG ('M','C','F','G'))
            Acpi::mcfg = table[i];
    }
}
