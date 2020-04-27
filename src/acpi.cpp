/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "acpi_dbg2.hpp"
#include "acpi_fadt.hpp"
#include "acpi_gtdt.hpp"
#include "acpi_iort.hpp"
#include "acpi_madt.hpp"
#include "acpi_rsdp.hpp"
#include "acpi_rsdt.hpp"
#include "acpi_spcr.hpp"
#include "compiler.hpp"
#include "hpt.hpp"
#include "uefi.hpp"

bool Acpi::init()
{
    auto rsdp = reinterpret_cast<Uefi *>(Kmem::sym_to_virt (&Uefi::uefi))->rsdp;

    uint64 rsdt;
    size_t size;

    if (!rsdp || !static_cast<Acpi_rsdp *>(Hpt::map (rsdp))->parse (rsdt, size))
        return false;

    if (rsdt)
        static_cast<Acpi_table_rsdt *>(Hpt::map (rsdt))->parse (rsdt, size);
    if (dbg2)
        static_cast<Acpi_table_dbg2 *>(Hpt::map (dbg2))->parse();
    if (spcr)
        static_cast<Acpi_table_spcr *>(Hpt::map (spcr))->parse();
    if (fadt)
        static_cast<Acpi_table_fadt *>(Hpt::map (fadt))->parse();
    if (gtdt)
        static_cast<Acpi_table_gtdt *>(Hpt::map (gtdt))->parse();
    if (iort)
        static_cast<Acpi_table_iort *>(Hpt::map (iort))->parse();
    if (madt)
        static_cast<Acpi_table_madt *>(Hpt::map (madt))->parse();

    return true;
}
