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

Sm::Sm (Refptr<Pd> &ref_pd, Kobject::Subtype t, uintptr_t v) : Kobject { Kobject::Type::SM, t }, pd { std::move (ref_pd) }, cnt { t == Kobject::Subtype::SM_REG ? v : 0 }, ise { 0 }, iid { t == Kobject::Subtype::SM_INT ? static_cast<uint32_t>(v) : 0 }
{
    trace (TRACE_CREATE, "SM:%p created (PD:%p %s:%#lx)", static_cast<void *>(this), static_cast<void *>(pd), t == Kobject::Subtype::SM_REG ? "CNT" : "IID", v);
}

Sm *Sm::create (Status &s, Pd *pd, Kobject::Subtype t, uintptr_t v)
{
    // Acquire reference
    Refptr<Pd> ref_pd { pd };

    // Failed to acquire reference
    if (!ref_pd) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new SM object
        auto const obj { new (ref_pd->sm_cache) Sm { ref_pd, t, v } };

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
