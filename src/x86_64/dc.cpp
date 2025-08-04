/*
 * Device Context (DC)
 *
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

#include "dc.hpp"
#include "stdio.hpp"

Dc::Dc (Refptr<Pd> &ref_pd, uint64_t t, uint64_t d, uint64_t i) : Kobject { Kobject::Type::DC }, Dc_state { t, d, i }, pd { std::move (ref_pd) }
{
    trace (TRACE_CREATE, "DC:%p created (PD:%p)", static_cast<void *>(this), static_cast<void *>(pd));
}

Dc *Dc::create (Status &s, Pd *pd, uint64_t t, uint64_t d, uint64_t i)
{
    // Acquire reference
    Refptr<Pd> ref_pd { pd };

    // Failed to acquire reference
    if (!ref_pd) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new DC object
        auto const obj { new (ref_pd->dc_cache) Dc { ref_pd, t, d, i } };

        // If creation succeeded, then reference must have been consumed
        if (obj) [[likely]] {
            assert (!ref_pd);
            return obj;
        }

        // Failed to create DC object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Dc::destroy()
{
    auto &cache { pd->dc_cache };

    this->~Dc();

    operator delete (this, cache);
}
