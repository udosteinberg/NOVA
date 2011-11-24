/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pci::cache (sizeof (Pci), 8);

unsigned    Pci::bus_base;
Paddr       Pci::cfg_base;
size_t      Pci::cfg_size;
Pci *       Pci::list;

Pci::Pci (unsigned b, unsigned d, unsigned f, unsigned l) : reg_base (hwdev_addr -= PAGE_SIZE), rid (static_cast<uint16>(b << 8 | d << 3 | f)), level (static_cast<uint16>(l)), next (nullptr)
{
    Pci **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;

    Pd::kern.Space_mem::insert (reg_base, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, cfg_base + (rid << PAGE_BITS));
}

unsigned Pci::init (unsigned b, unsigned l)
{
    unsigned m = b;

    for (unsigned d = 0; d < 32; d++) {

        for (unsigned f = 0; f < 8; f++) {

            Bdf bdf (b, d, f);

            if (bdf.read<uint32>(0x0) == 0xffffffff)
                continue;

            new Pci (b, d, f, l);

            unsigned htype = bdf.read<uint8>(0xe);

            // PCI bridge
            if ((htype & 0x7f) == 1)
                m = max (init (bdf.read<uint8>(0x19), l + 1), m);

            // Multi-function device
            if (!f && !(htype & 0x80))
                break;
        }
    }

    return m;
}
