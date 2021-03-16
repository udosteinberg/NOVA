/*
 * Portal (PT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "ec.hpp"
#include "pt.hpp"
#include "stdio.hpp"

Pt::Pt (Refptr<Ec> &ref_ec, uintptr_t i) : Kobject { Kobject::Type::PT }, ec { std::move (ref_ec) }, ip { i }
{
    trace (TRACE_CREATE, "PT:%p created (EC:%p IP:%#lx)", static_cast<void *>(this), static_cast<void *>(ec), ip);
}

Pt *Pt::create (Status &s, Ec *ec, uintptr_t ip)
{
    // Acquire reference
    Refptr<Ec> ref_ec { ec };

    // Failed to acquire reference
    if (!ref_ec) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new PT object
        auto const obj { new (ref_ec->get_pd()->pt_cache) Pt { ref_ec, ip } };

        // If creation succeeded, then reference must have been consumed
        if (obj) [[likely]] {
            assert (!ref_ec);
            return obj;
        }

        // Failed to create PT object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

// Called when refcount on PT reaches 0
void Pt::collect()
{
    trace (TRACE_DESTROY, "KOBJ: PT %p reached refcount zero", static_cast<void *>(this));

    // Scrub internal pointers
}

void Pt::destroy()
{
    trace (TRACE_DESTROY, "KOBJ: PT %p is being destructed", static_cast<void *>(this));

    auto &cache { ec->get_pd()->pt_cache };

    this->~Pt();

    operator delete (this, cache);
}
