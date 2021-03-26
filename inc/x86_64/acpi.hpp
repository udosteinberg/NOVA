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
#include "acpi_table_dmar.hpp"
#include "acpi_table_fadt.hpp"
#include "acpi_table_hpet.hpp"
#include "acpi_table_madt.hpp"
#include "acpi_table_mcfg.hpp"
#include "acpi_table_rsdp.hpp"
#include "acpi_table_rsdt.hpp"
#include "hpt.hpp"

class Acpi
{
    friend class Acpi_table;
    friend class Acpi_table_fadt;

    private:
        static inline uint64 dmar { 0 }, facs { 0 }, fadt { 0 }, hpet { 0 }, madt { 0 }, mcfg { 0 };
        static inline uint32 fflg { 0 };    // Feature Flags
        static inline uint16 bflg { 0 };    // Boot Flags

        static constexpr struct
        {
            uint32 const    sig;
            uint64 &        var;
        } tables[] =
        {
            { Acpi_table::sig ("APIC"), madt },
            { Acpi_table::sig ("DMAR"), dmar },
            { Acpi_table::sig ("FACP"), fadt },
            { Acpi_table::sig ("HPET"), hpet },
            { Acpi_table::sig ("MCFG"), mcfg },
        };

        static inline uintptr_t rsdp_probe (uintptr_t map, uintptr_t off, size_t len)
        {
            for (auto ptr = map + off; ptr < map + off + len; ptr += 16)
                if (reinterpret_cast<Acpi_table_rsdp *>(ptr)->valid())
                    return ptr - map;

            return 0;
        }

        /*
         * OSPM finds the Root System Description Pointer (RSDP) structure by
         * searching physical memory ranges on 16-byte boundaries for a
         * valid Root System Description Pointer structure signature and
         * checksum match as follows:
         * - The first 1 KB of the Extended BIOS Data Area (EBDA). For EISA
         *   or MCA systems, the EBDA can be found in the two-byte location
         *   40:0Eh on the BIOS data area.
         * - The BIOS read-only memory space between 0E0000h and 0FFFFFh.
         */
        static inline uintptr_t rsdp_find()
        {
            uintptr_t addr, map = reinterpret_cast<uintptr_t>(Hpt::map (0));

            if (!(addr = rsdp_probe (map, *reinterpret_cast<uint16 *>(map + 0x40e) << 4, 0x400)) &&
                !(addr = rsdp_probe (map, 0xe0000, 0x20000)))
                return 0;

            return addr;
        }

        static inline void parse_arch_tables()
        {
            if (hpet)
                static_cast<Acpi_table_hpet *>(Hpt::map (hpet))->parse();
            if (madt)
                static_cast<Acpi_table_madt *>(Hpt::map (madt))->parse();
            if (mcfg)
                static_cast<Acpi_table_mcfg *>(Hpt::map (mcfg))->parse();
            if (dmar)
                static_cast<Acpi_table_dmar *>(Hpt::map (dmar))->parse();
        }

    public:
        static bool init();
};
