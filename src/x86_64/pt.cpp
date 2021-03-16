/*
 * Portal (PT)
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

#include "ec.hpp"
#include "pt.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Pt::cache { sizeof (Pt), Kobject::alignment };

Pt::Pt (Ec *e, uintptr_t i) : Kobject (Kobject::Type::PT), ec (e), ip (i)
{
    trace (TRACE_CREATE, "PT:%p created (EC:%p IP:%#lx)", static_cast<void *>(this), static_cast<void *>(e), ip);
}
