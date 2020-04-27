/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "acpi_table_gtdt.hpp"
#include "acpi_table_iort.hpp"
#include "ptab_hpt.hpp"

class Acpi_arch
{
    protected:
        static constexpr auto rsdp_find() { return 0; }

        static void parse_tables()
        {
            if (gtdt)
                static_cast<Acpi_table_gtdt *>(Hptp::map (MMAP_GLB_MAP0, gtdt))->parse();
            if (iort)
                static_cast<Acpi_table_iort *>(Hptp::map (MMAP_GLB_MAP0, iort))->parse();
        }

        static void wake_prepare() {}

        static inline uint64_t dbg2 { 0 }, facs { 0 }, fadt { 0 }, gtdt { 0 }, iort { 0 }, madt { 0 }, spcr { 0 }, srat { 0 };

    public:
        static constexpr struct
        {
            uint32_t const  sig;
            uint64_t &      var;
        } tables[] =
        {
            { Signature::value ("APIC"), madt },
            { Signature::value ("DBG2"), dbg2 },
            { Signature::value ("FACP"), fadt },
            { Signature::value ("GTDT"), gtdt },
            { Signature::value ("IORT"), iort },
            { Signature::value ("SPCR"), spcr },
            { Signature::value ("SRAT"), srat },
        };
};
