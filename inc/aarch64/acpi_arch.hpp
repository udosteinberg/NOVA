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

#pragma once

#include "acpi_table.hpp"
#include "acpi_table_dbg2.hpp"
#include "acpi_table_facs.hpp"
#include "acpi_table_fadt.hpp"
#include "acpi_table_gtdt.hpp"
#include "acpi_table_iort.hpp"
#include "acpi_table_madt.hpp"
#include "acpi_table_rsdp.hpp"
#include "acpi_table_rsdt.hpp"
#include "acpi_table_spcr.hpp"
#include "ptab_hpt.hpp"

class Acpi_arch
{
    protected:
        static inline uintptr_t rsdp_find() { return 0; }

        static inline void parse_tables()
        {
            if (dbg2)
                static_cast<Acpi_table_dbg2 *>(Hptp::map (dbg2))->parse();
            if (spcr)
                static_cast<Acpi_table_spcr *>(Hptp::map (spcr))->parse();
            if (gtdt)
                static_cast<Acpi_table_gtdt *>(Hptp::map (gtdt))->parse();
            if (iort)
                static_cast<Acpi_table_iort *>(Hptp::map (iort))->parse();
            if (madt)
                static_cast<Acpi_table_madt *>(Hptp::map (madt))->parse();
        }

        static inline uint64 dbg2 { 0 }, facs { 0 }, fadt { 0 }, gtdt { 0 }, iort { 0 }, madt { 0 }, spcr { 0 };

    public:
        static constexpr struct
        {
            uint32 const    sig;
            uint64 &        var;
        } tables[] =
        {
            { Acpi_table::sig ("APIC"), madt },
            { Acpi_table::sig ("DBG2"), dbg2 },
            { Acpi_table::sig ("FACP"), fadt },
            { Acpi_table::sig ("GTDT"), gtdt },
            { Acpi_table::sig ("IORT"), iort },
            { Acpi_table::sig ("SPCR"), spcr },
        };
};
