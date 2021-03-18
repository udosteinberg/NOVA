/*
 * PCI Configuration Space
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

#include "pci.hpp"
#include "ptab_hpt.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Pci::cache { sizeof (Pci), alignof (Pci) };

struct Pci::quirk_map Pci::map[] =
{
};

Pci::Pci (unsigned r, unsigned l) : List (list), reg_base (mmap), rid (static_cast<uint16>(r)), lev (static_cast<uint16>(l))
{
    mmap += PAGE_SIZE;

    Hptp::master_map (reg_base, cfg_base + (rid << PAGE_BITS), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    for (unsigned i { 0 }; i < sizeof map / sizeof *map; i++)
        if (cfg_read (Register32::DID_VID) == map[i].didvid)
            (this->*map[i].func)();
}

void Pci::init (unsigned b, unsigned l)
{
    for (auto r { b << 8 }; r < (b + 1) << 8; r++) {

        if (*static_cast<uint32 *>(Hptp::map (cfg_base + (r << PAGE_BITS))) == BIT_RANGE (31, 0))
            continue;

        auto const pci { new Pci (r, l) };
        auto const hdr { pci->cfg_read (Register8::HTYPE) };

        // PCI-PCI Bridge
        if ((hdr & BIT_RANGE (6, 0)) == 1)
            init (pci->cfg_read (Register8::BUS_SEC), l + 1);

        // Multi-Function Device
        if (!(r & 0x7) && !(hdr & BIT (7)))
            r += 7;
    }
}
