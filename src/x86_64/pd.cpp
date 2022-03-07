/*
 * Protection Domain (PD)
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

#include "fpu.hpp"
#include "pd_kern.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), Kobject::alignment);

Atomic<Pd *> Pd::current { nullptr };

// FIXME: Bitmap allocation via factory
Pd::Pd() : Kobject (Kobject::Type::PD), Space_pio (new Bitmap_pio, new Bitmap_pio), Space_msr (new Bitmap_msr), fpu_cache (Fpu::size, 64)
{
    trace (TRACE_CREATE, "PD:%p created", static_cast<void *>(this));

    if (Space_pio::bmp_hst())
        Space_mem::update (MMAP_SPC_IOP, Kmem::ptr_to_phys (Space_pio::bmp_hst()), 1, Paging::Permissions (Paging::W | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}
