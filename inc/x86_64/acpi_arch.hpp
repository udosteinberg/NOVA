/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "acpi_table_dmar.hpp"
#include "acpi_table_facs.hpp"
#include "acpi_table_hpet.hpp"
#include "acpi_table_lpit.hpp"
#include "acpi_table_rsdp.hpp"
#include "ptab_hpt.hpp"
#include "string.hpp"

class Acpi_arch
{
    private:
        static inline constinit char sra[128];

        static uintptr_t rsdp_probe (uintptr_t map, uintptr_t off, size_t len)
        {
            for (auto ptr { map + off }; ptr < map + off + len; ptr += 16)
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
        static uintptr_t rsdp_find()
        {
            uintptr_t addr, map { reinterpret_cast<uintptr_t>(Hptp::map (MMAP_GLB_MAP0, 0)) };

            if ((addr = rsdp_probe (map, *reinterpret_cast<uint16_t const *>(map + 0x40e) << 4, 0x400)) ||
                (addr = rsdp_probe (map, 0xe0000, 0x20000)))
                return addr;

            return 0;
        }

        static void parse_tables()
        {
            void const *ptr;

            if ((ptr = Acpi_table::map (hpet, 0))) [[likely]]
                static_cast<Acpi_table_hpet const *>(ptr)->parse();
            if ((ptr = Acpi_table::map (dmar, 0))) [[likely]]
                static_cast<Acpi_table_dmar const *>(ptr)->parse();
            if ((ptr = Acpi_table::map (lpit, 0))) [[likely]]
                static_cast<Acpi_table_lpit const *>(ptr)->parse();
        }

        static void wake_prepare()
        {
            extern char __init_aps, __init_aps__, __wake_vec;

            size_t const len = &__init_aps__ - &__init_aps;

            // Ensure the save/restore area is large enough
            assert (len <= sizeof (sra));

            auto const ptr { Hptp::map (MMAP_GLB_MAP0, sipi, Paging::W) };
            memcpy (sra, ptr, sizeof (sra));
            memcpy (ptr, reinterpret_cast<void *>(Kmem::sym_to_virt (&__init_aps)), len);

            if (facs)
                static_cast<Acpi_table_facs *>(Hptp::map (MMAP_GLB_MAP0, facs, Paging::W))->set_wake (sipi + static_cast<uint32_t>(&__wake_vec - &__init_aps));
        }

        static inline constinit uint64_t dbg2 { 0 }, dmar { 0 }, facs { 0 }, fadt { 0 }, hest { 0 }, hpet { 0 }, lpit { 0 }, madt { 0 }, mcfg { 0 }, spcr { 0 }, srat { 0 }, tpm2 { 0 }, xsdt { 0 };

    public:
        static constexpr auto sipi { PAGE_SIZE (0) };

        // SIPI vector must be a PAGE-aligned address < 1MB
        static_assert (sipi < BIT (20) && !(sipi & OFFS_MASK (0)));

        static constexpr struct
        {
            uint32_t const  sig;        // Signature
            uint32_t const  len;        // Minimum Length
            uint64_t &      var;
        } tables[] {
            { Signature::u32 ("APIC"),  44, madt },
            { Signature::u32 ("DBG2"),  44, dbg2 },
            { Signature::u32 ("DMAR"),  48, dmar },
            { Signature::u32 ("FACP"), 244, fadt },
            { Signature::u32 ("HEST"),  40, hest },
            { Signature::u32 ("HPET"),  56, hpet },
            { Signature::u32 ("LPIT"),  36, lpit },
            { Signature::u32 ("MCFG"),  44, mcfg },
            { Signature::u32 ("RSDT"),  36, xsdt },
            { Signature::u32 ("SPCR"),  80, spcr },
            { Signature::u32 ("SRAT"),  48, srat },
            { Signature::u32 ("TPM2"),  52, tpm2 },
            { Signature::u32 ("XSDT"),  36, xsdt },
        };

        static void wake_restore()
        {
            memcpy (Hptp::map (MMAP_GLB_MAP0, sipi, Paging::W), sra, sizeof (sra));
        }
};
