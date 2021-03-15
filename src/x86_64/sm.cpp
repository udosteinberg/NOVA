/*
 * Semaphore (SM)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "sm.hpp"
#include "stdio.hpp"

Sm::Sm (Refptr<Pd> &ref_pd, uintptr_t v, void *p) : Kobject { Kobject::Type::SM, p ? Kobject::Subtype::SM_INT : Kobject::Subtype::SM_REG }, pd { std::move (ref_pd) }, cnt { p ? 0 : v }, ptr { p }, iid { static_cast<iid_t>(p ? v : 0) }
{
    trace (TRACE_CREATE, "SM:%p created (PD:%p %s:%#lx)", static_cast<void *>(this), static_cast<void *>(pd), p ? "GSI" : "CNT", v);
}

Sm *Sm::create (Status &s, Pd *pd, uintptr_t v, void *p)
{
    // Acquire reference
    Refptr<Pd> ref_pd { pd };

    // Failed to acquire reference
    if (!ref_pd) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new SM object
        auto const obj { new (ref_pd->sm_cache) Sm { ref_pd, v, p } };

        // If creation succeeded, then reference must have been consumed
        if (obj) [[likely]] {
            assert (!ref_pd);
            return obj;
        }

        // Failed to create SM object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Sm::collect()
{
    trace (TRACE_DESTROY, "KOBJ: SM %p reached refcount zero", static_cast<void *>(this));
}

void Sm::destroy()
{
    auto &cache { pd->sm_cache };

    this->~Sm();

    operator delete (this, cache);
}
