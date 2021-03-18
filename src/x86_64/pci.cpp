/*
 * PCI Configuration Space
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

#include "pci.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Pci::cache { sizeof (Pci), alignof (Pci) };

struct Pci::quirk_map Pci::map[] =
{
};

Pci::Pci (pci_t s, unsigned l) : List (list), reg_base (mmap), sid (s), lev (static_cast<uint16_t>(l))
{
    mmap += PAGE_SIZE;

    Hptp::master_map (reg_base, cfg_base + (sid << PAGE_BITS), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    auto const didvid { cfg_read (Register32::DID_VID) };

    trace (TRACE_PCI, "PCIE: Device %02x:%02x.%x is %04x:%04x (%#010x)", bus (sid), dev (sid), fun (sid), static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16), cfg_read (Register32::CCP_RID));

    for (unsigned i { 0 }; i < sizeof map / sizeof *map; i++)
        if (map[i].didvid == didvid)
            (this->*map[i].func)();
}

void Pci::init (unsigned b, unsigned l)
{
    for (pci_t s { bdf (b) }; s < bdf (b + 1); s++) {

        if (*static_cast<uint32_t *>(Hptp::map (cfg_base + (s << PAGE_BITS))) == BIT_RANGE (31, 0))
            continue;

        auto const pci { new Pci (s, l) };
        auto const hdr { pci->cfg_read (Register8::HTYPE) };

        // PCI-PCI Bridge
        if ((hdr & BIT_RANGE (6, 0)) == 1)
            init (pci->cfg_read (Register8::BUS_SEC), l + 1);

        // Multi-Function Device
        if (!(s & 0x7) && !(hdr & BIT (7)))
            s += 7;
    }
}
