/*
 * Host Memory Space
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
    Space_obj::nova.insert (Space_obj::Selector::NOVA_HST, Capability { this, std::to_underlying (Capability::Perm_sp::TAKE) });

    // Highest mappable PA
    constexpr uintptr_t max_addr { selectors << PAGE_BITS };

    // Compute image addresses within mappable PA bounds
    auto const s { min (max_addr, Kmem::sym_to_phys (&NOVA_HPAS)) };
    auto const e { min (max_addr, Multiboot::ea) };

    access_ctrl (0, s, Paging::Permissions (Paging::U | Paging::API));
    access_ctrl (e, max_addr - e, Paging::Permissions (Paging::U | Paging::API));
}

Space_hst *Space_hst::create (Status &s, Pd *pd)
{
    // Acquire reference
    Refptr<Pd> ref_pd { pd };

    // Failed to acquire reference
    if (!ref_pd) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new HST object
        auto const obj { new (ref_pd->hst_cache) Space_hst { ref_pd } };

        // If creation succeeded, then reference must have been consumed
        if (obj) [[likely]] {

            assert (!ref_pd);

            if (obj->nptp.root_init()) [[likely]]
                return obj;

            operator delete (obj, ref_pd->hst_cache);
        }

        // Failed to create HST object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Space_hst::destroy()
{
    auto &cache { get_pd()->hst_cache };

    this->~Space_hst();

    operator delete (this, cache);
}
