/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "acpi.hpp"
#include "uefi.hpp"

bool Acpi::init()
{
    auto &rsdp = reinterpret_cast<Uefi *>(Kmem::sym_to_virt (&Uefi::uefi))->rsdp;

    if (!rsdp && !(rsdp = rsdp_find()))
        return false;

    uint64 rsdt;
    size_t size;

    if (static_cast<Acpi_table_rsdp *>(Hpt::map (rsdp))->parse (rsdt, size) == false)
        return false;

    if (rsdt)
        static_cast<Acpi_table_rsdt *>(Hpt::map (rsdt))->parse (rsdt, size);
    if (fadt)
        static_cast<Acpi_table_fadt *>(Hpt::map (fadt))->parse();

    parse_arch_tables();

    return true;
}
