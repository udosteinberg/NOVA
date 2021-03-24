/*
 * Scheduling Context (SC)
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

#include "ec.hpp"
#include "stc.hpp"
#include "stdio.hpp"

Sc::Sc (Refptr<Ec> &ref_ec, cpu_t n, uint16_t b, uint8_t p, cos_t c) : Kobject { Kobject::Type::SC }, ec { std::move (ref_ec) }, budget { Stc::ms_to_ticks (b) }, cpu { n }, cos { c }, prio { p }
{
    trace (TRACE_CREATE, "SC:%p created (EC:%p CPU:%u Budget:%ums Prio:%u COS:%u)", static_cast<void *>(this), static_cast<void *>(ec), cpu, b, p, c);
}

Sc *Sc::create (Status &s, Ec *ec, cpu_t cpu, uint16_t budget, uint8_t prio, cos_t cos)
{
    // Acquire reference
    Refptr<Ec> ref_ec { ec };

    // Failed to acquire reference
    if (!ref_ec) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new SC object
        auto const obj { new (ref_ec->get_pd()->sc_cache) Sc { ref_ec, cpu, budget, prio, cos } };

        // If creation succeeded, then reference must have been consumed
        if (obj) [[likely]] {
            assert (!ref_ec);
            return obj;
        }

        // Failed to create SC object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Sc::destroy()
{
    auto &cache { ec->get_pd()->sc_cache };

    this->~Sc();

    operator delete (this, cache);
}
