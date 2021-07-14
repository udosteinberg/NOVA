/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "acpi_table_facs.hpp"
#include "acpi_table_fadt.hpp"
#include "acpi_table_hpet.hpp"
#include "acpi_table_lpit.hpp"
#include "acpi_table_madt.hpp"
#include "acpi_table_mcfg.hpp"
#include "acpi_table_rsdp.hpp"
#include "acpi_table_rsdt.hpp"
#include "extern.hpp"
#include "ptab_hpt.hpp"
#include "string.hpp"

class Acpi_arch
{
    private:
        static constexpr auto sipi { 0x1000 };

        static inline char sra[128];

        static inline uintptr_t rsdp_probe (uintptr_t map, uintptr_t off, size_t len)
        {
            for (auto ptr = map + off; ptr < map + off + len; ptr += 16)
                if (reinterpret_cast<Acpi_table_rsdp *>(ptr)->valid())
                    return ptr - map;

            return 0;
        }

    protected:
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
            uintptr_t addr, map = reinterpret_cast<uintptr_t>(Hptp::map (0));

            if (!(addr = rsdp_probe (map, *reinterpret_cast<uint16 *>(map + 0x40e) << 4, 0x400)) &&
                !(addr = rsdp_probe (map, 0xe0000, 0x20000)))
                return 0;

            return addr;
        }

        static inline void parse_tables()
        {
            if (hpet)
                static_cast<Acpi_table_hpet *>(Hptp::map (hpet))->parse();
            if (madt)
                static_cast<Acpi_table_madt *>(Hptp::map (madt))->parse();
            if (mcfg)
                static_cast<Acpi_table_mcfg *>(Hptp::map (mcfg))->parse();
            if (dmar)
                static_cast<Acpi_table_dmar *>(Hptp::map (dmar))->parse();
            if (lpit)
                static_cast<Acpi_table_lpit *>(Hptp::map (lpit))->parse();
        }

        static inline void wake_prepare()
        {
            size_t const len = &__desc_gdt__ - &__init_aps;

            // Ensure the save/restore area is large enough
            assert (len <= sizeof (sra));

            auto ptr = Hptp::map (sipi, true);
            memcpy (sra, ptr, sizeof (sra));
            memcpy (ptr, reinterpret_cast<void *>(Kmem::sym_to_virt (&__init_aps)), len);

            if (facs)
                static_cast<Acpi_table_facs *>(Hptp::map (facs, true))->set_wake (sipi + static_cast<uint32>(&__wake_vec - &__init_aps));
        }

        static inline uint64 dmar { 0 }, facs { 0 }, fadt { 0 }, hpet { 0 }, lpit { 0 }, madt { 0 }, mcfg { 0 };

    public:
        static constexpr struct
        {
            uint32 const    sig;
            uint64 &        var;
        } tables[] =
        {
            { Acpi_header::sig_value ("APIC"), madt },
            { Acpi_header::sig_value ("DMAR"), dmar },
            { Acpi_header::sig_value ("FACP"), fadt },
            { Acpi_header::sig_value ("HPET"), hpet },
            { Acpi_header::sig_value ("LPIT"), lpit },
            { Acpi_header::sig_value ("MCFG"), mcfg },
        };

        static inline void wake_restore()
        {
            memcpy (Hptp::map (sipi, true), sra, sizeof (sra));
        }
};
