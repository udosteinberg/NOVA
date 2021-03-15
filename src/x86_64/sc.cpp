/*
 * Scheduling Context (SC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

INIT_PRIORITY (PRIO_SLAB) Slab_cache Sc::cache { sizeof (Sc), Kobject::alignment };

Sc::Sc (Refptr<Ec> &e, cpu_t n, uint16_t b, uint8_t p, cos_t c) : Kobject { Kobject::Type::SC }, ec { std::move (e) }, budget { Stc::ms_to_ticks (b) }, cpu { n }, cos { c }, prio { p }
{
    trace (TRACE_CREATE, "SC:%p created (EC:%p CPU:%u Budget:%ums Prio:%u COS:%u)", static_cast<void *>(this), static_cast<void *>(ec), cpu, b, p, c);
}
