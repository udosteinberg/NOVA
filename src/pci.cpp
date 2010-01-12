/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2010, Udo Steinberg <udo@hypervisor.org>
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

Slab_cache Pci::cache (sizeof (Pci), 8);
Paddr Pci::cfg_base;
Pci *Pci::list;

Pci::Pci (unsigned b, unsigned d, unsigned f, unsigned l) : reg_base (hwdev_addr -= PAGE_SIZE), rid (static_cast<uint16>(b << 8 | d << 3 | f)), level (static_cast<uint16>(l)), next (0)
{
    Pci **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;

    Pd::kern.Space_mem::insert (hwdev_addr, 0,
                                Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                                 Ptab::ATTR_GLOBAL      |
                                                 Ptab::ATTR_UNCACHEABLE |
                                                 Ptab::ATTR_WRITABLE),
                                cfg_base + (rid << PAGE_BITS));
}

void Pci::init (unsigned b, unsigned l)
{
    for (unsigned d = 0; d < 32; d++) {

        for (unsigned f = 0; f < 8; f++) {

            Bdf bdf (b, d, f);

            if (bdf.read<uint32>(0x0) == ~0ul)
                continue;

            new Pci (b, d, f, l);

            unsigned htype = bdf.read<uint8>(0xe);

            // PCI bridge
            if ((htype & 0x7f) == 1)
                init (bdf.read<uint8>(0x19), l + 1);

            // Multi-function device
            if (!f && !(htype & 0x80))
                break;
        }
    }
}
