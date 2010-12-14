/*
 * Memory Type Range Registers (MTRR)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#include "initprio.h"
#include "msr.h"
#include "mtrr.h"

unsigned Mtrr::count;
unsigned Mtrr::dtype;
Mtrr *   Mtrr::list;

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Mtrr::cache (sizeof (Mtrr), 8);

Mtrr::Mtrr (uint64 b, uint64 m) : base (b), mask (m)
{
    Mtrr **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;
}

void Mtrr::init()
{
    count = Msr::read<uint32>(Msr::IA32_MTRR_CAP) & 0xff;
    dtype = Msr::read<uint32>(Msr::IA32_MTRR_DEF_TYPE) & 0xff;

    for (unsigned i = 0; i < count; i++)
        new Mtrr (Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_PHYS_BASE + 2 * i)),
                  Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_PHYS_MASK + 2 * i)));
}

unsigned Mtrr::memtype (uint64 phys)
{
    if (phys < 0x80000)
        return static_cast<unsigned>(Msr::read<uint64>(Msr::IA32_MTRR_FIX64K_BASE) >>
                                    (phys >> 13 & 0x38)) & 0xff;
    if (phys < 0xc0000)
        return static_cast<unsigned>(Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_FIX16K_BASE + (phys >> 17 & 0x1))) >>
                                    (phys >> 11 & 0x38)) & 0xff;
    if (phys < 0x100000)
        return static_cast<unsigned>(Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_FIX4K_BASE  + (phys >> 15 & 0x7))) >>
                                    (phys >>  9 & 0x38)) & 0xff;

    for (Mtrr *mtrr = list; mtrr; mtrr = mtrr->next)
        if ((mtrr->mask & 0x800) && ((phys ^ mtrr->base) & mtrr->mask) >> 12 == 0)
            return static_cast<unsigned>(mtrr->base) & 0xff;

    return dtype;
}
