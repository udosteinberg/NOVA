/*
 * Memory Type Range Registers (MTRR)
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

#include "memory.hpp"
#include "msr.hpp"
#include "mtrr.hpp"

unsigned Mtrr::count;
unsigned Mtrr::dtype;
Mtrr *   Mtrr::list;

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Mtrr::cache (sizeof (Mtrr), 8);

void Mtrr::init()
{
    count = Msr::read (Msr::Register::IA32_MTRR_CAP) & 0xff;
    dtype = Msr::read (Msr::Register::IA32_MTRR_DEF_TYPE) & 0xff;

    for (unsigned i = 0; i < count; i++)
        new Mtrr (Msr::read (Msr::Array::IA32_MTRR_PHYS_BASE, 2, i),
                  Msr::read (Msr::Array::IA32_MTRR_PHYS_MASK, 2, i));
}

unsigned Mtrr::memtype (uint64 phys, uint64 &next)
{
    if (phys < 0x80000) {
        next = 1 + (phys | 0xffff);
        return static_cast<unsigned>(Msr::read (Msr::Array::IA32_MTRR_FIX64K_BASE, 1, 0) >>
                                    (phys >> 13 & 0x38)) & 0xff;
    }

    if (phys < 0xc0000) {
        next = 1 + (phys | 0x3fff);
        return static_cast<unsigned>(Msr::read (Msr::Array::IA32_MTRR_FIX16K_BASE, 1, phys >> 17 & 0x1) >>
                                    (phys >> 11 & 0x38)) & 0xff;
    }

    if (phys < 0x100000) {
        next = 1 + (phys | 0xfff);
        return static_cast<unsigned>(Msr::read (Msr::Array::IA32_MTRR_FIX4K_BASE, 1, phys >> 15 & 0x7) >>
                                    (phys >>  9 & 0x38)) & 0xff;
    }

    unsigned type = ~0U; next = ~0ULL;

    for (Mtrr *mtrr = list; mtrr; mtrr = mtrr->next) {

        if (!(mtrr->mask & 0x800))
            continue;

        uint64 base = mtrr->base & ~OFFS_MASK;

        if (phys < base)
            next = min (next, base);

        else if (((phys ^ mtrr->base) & mtrr->mask) >> PAGE_BITS == 0) {
            next = min (next, base + mtrr->size());
            type = min (type, static_cast<unsigned>(mtrr->base) & 0xff);
        }
    }

    return type == ~0U ? dtype : type;
}
