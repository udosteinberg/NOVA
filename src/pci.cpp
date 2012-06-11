/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "pci.h"
#include "pd.h"
#include "stdio.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pci::cache (sizeof (Pci), 8);

unsigned    Pci::bus_base;
Paddr       Pci::cfg_base;
size_t      Pci::cfg_size;
Pci *       Pci::list;

Pci::Pci (unsigned r, unsigned l) : List<Pci> (list), reg_base (hwdev_addr -= PAGE_SIZE), rid (static_cast<uint16>(r)), lev (static_cast<uint16>(l))
{
    Pd::kern.Space_mem::insert (reg_base, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, cfg_base + (rid << PAGE_BITS));
}

void Pci::init (unsigned b, unsigned l)
{
    for (unsigned r = b << 8; r < (b + 1) << 8; r++) {

        if (*static_cast<uint32 *>(Hpt::remap (cfg_base + (r << PAGE_BITS))) == ~0U)
            continue;

        Pci *p = new Pci (r, l);

        unsigned h = p->read<uint8>(REG_HDR);

        if ((h & 0x7f) == 1)
            init (p->read<uint8>(REG_SBUSN), l + 1);

        if (!(r & 0x7) && !(h & 0x80))
            r += 7;
    }
}
