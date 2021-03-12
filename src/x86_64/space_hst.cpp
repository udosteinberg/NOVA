/*
 * Host Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "multiboot.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"

INIT_PRIORITY (PRIO_SPACE_MEM) ALIGNED (Kobject::alignment) Space_hst Space_hst::nova;

Space_hst *Space_hst::current { nullptr };

/*
 * Constructor (NOVA HST Space)
 */
Space_hst::Space_hst() : Space_mem { Kobject::Subtype::HST }
{
    Space_obj::nova.insert (Space_obj::Selector::NOVA_HST, Capability (this, std::to_underlying (Capability::Perm_sp::TAKE)));

    // FIXME: Create an L1 PTAB for early sharing before CPUs plug themselves into the array. CPU preallocation will make this obsolete.
    (void) Hptp::master.walk (MMAP_GLB_CPUS, 1, true);

    nova.hptp = Hptp::master;

    auto const s { Kmem::sym_to_phys (&NOVA_HPAS) };
    auto const e { Multiboot::ea };

    access_ctrl (0, s, Paging::Permissions (Paging::U | Paging::API));
    access_ctrl (e, BIT64 (min (Memattr::obits, Hpt::ibits - 1)) - e, Paging::Permissions (Paging::U | Paging::API));
}

void Space_hst::init (cpu_t cpu)
{
    if (cpus.tas (cpu)) [[likely]]
        return;

    // Share global kernel memory
    loc[cpu].share_from_master (BASE_ADDR, MMAP_CPU);

    // Share CPU-local memory
    loc[cpu].share_from (nova.loc[cpu], MMAP_CPU, MMAP_SPC);
}
