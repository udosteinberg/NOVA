/*
 * Protection Domain
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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
#include "mtrr.hpp"
#include "pd.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), 32);

Atomic<Pd *>    Pd::current { nullptr };
ALIGNED(32) Pd  Pd::kern (&Pd::kern);
ALIGNED(32) Pd  Pd::root (&Pd::root, NUM_EXC, 0x1f);

Pd::Pd (Pd *) : Kobject (Kobject::Type::PD), fpu_cache (sizeof (Fpu), 16)
{
    hpt = Hptp::master;

    Mtrr::init();

    Space_mem::insert_root (0, LOAD_ADDR);
    Space_mem::insert_root (reinterpret_cast<mword>(&NOVA_HPAE), USER_ADDR);

    // HIP
    Space_mem::insert_root (Kmem::ptr_to_phys (&PAGE_H), Kmem::ptr_to_phys (&PAGE_H) + PAGE_SIZE, 1);

#if 0   // FIXME
    // I/O Ports
    Space_pio::addreg (0, 1UL << 16, 7);
#endif
}
