/*
 * Semaphore (SM)
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

#include "sm.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Sm::cache { sizeof (Sm), Kobject::alignment };

Sm::Sm (uintptr_t v, bool i) : Kobject { Kobject::Type::SM, i ? Kobject::Subtype::SM_INT : Kobject::Subtype::SM_REG }, cnt { i ? 0 : v }, gsi { static_cast<gsi_t>(i ? v : 0) }
{
    trace (TRACE_CREATE, "SM:%p created (%s:%#lx)", static_cast<void *>(this), i ? "GSI" : "CNT", v);
}
