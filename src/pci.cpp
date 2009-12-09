/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009, Udo Steinberg <udo@hypervisor.org>
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
#include "stdio.h"

Slab_cache Pci::cache (sizeof (Pci), 8);

Pci *Pci::list;

Pci::Pci (unsigned b, unsigned d, unsigned f, unsigned l) : Bdf (b, d, f), next (0), level (l)
{
    Pci **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;
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
