/*
 * Host Memory Space
 *
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

#include "extern.hpp"
#include "multiboot.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"

INIT_PRIORITY (PRIO_SPACE_MEM) ALIGNED (Kobject::alignment) Space_hst Space_hst::nova;

/*
 * Constructor (NOVA HST Space)
 */
Space_hst::Space_hst() : Space_mem { Kobject::Subtype::HST }
{
    Space_obj::nova.insert (Space_obj::Selector::NOVA_HST, Capability (this, std::to_underlying (Capability::Perm_sp::TAKE)));

    auto const s { Kmem::sym_to_phys (&NOVA_HPAS) };
    auto const e { Multiboot::ea };

    access_ctrl (0, s, Paging::Permissions (Paging::U | Paging::API));
    access_ctrl (e, (selectors << PAGE_BITS) - e, Paging::Permissions (Paging::U | Paging::API));
}
