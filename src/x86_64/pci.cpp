/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "extern.hpp"
#include "pci.hpp"
#include "pd.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pci::cache (sizeof (Pci), 8);

unsigned    Pci::bus_base;
Paddr       Pci::cfg_base;
size_t      Pci::cfg_size;
Pci *       Pci::list;

struct Pci::quirk_map Pci::map[] =
{
};

Pci::Pci (unsigned r, unsigned l) : List<Pci> (list), reg_base (hwdev_addr -= PAGE_SIZE), rid (static_cast<uint16>(r)), lev (static_cast<uint16>(l))
{
    Hptp::master_map (reg_base, cfg_base + (rid << PAGE_BITS), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    for (unsigned i = 0; i < sizeof map / sizeof *map; i++)
        if (read<uint16>(REG_VID) == map[i].vid && read<uint16>(REG_DID) == map[i].did)
            (this->*map[i].func)();
}

void Pci::init (unsigned b, unsigned l)
{
    for (unsigned r = b << 8; r < (b + 1) << 8; r++) {

        if (*static_cast<uint32 *>(Hptp::map (Hptp::Remap::MAP0, cfg_base + (r << PAGE_BITS))) == ~0U)
            continue;

        Pci *p = new Pci (r, l);

        unsigned h = p->read<uint8>(REG_HDR);

        if ((h & 0x7f) == 1)
            init (p->read<uint8>(REG_SBUSN), l + 1);

        if (!(r & 0x7) && !(h & 0x80))
            r += 7;
    }
}
