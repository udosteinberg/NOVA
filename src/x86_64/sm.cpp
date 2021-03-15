/*
 * Semaphore (SM)
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

#include "sm.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Sm::cache { sizeof (Sm), Kobject::alignment };

Sm::Sm (uintptr_t v, void *p) : Kobject { Kobject::Type::SM, p ? Kobject::Subtype::SM_INT : Kobject::Subtype::SM_REG }, cnt { p ? 0 : v }, ptr { p }, iid { static_cast<iid_t>(p ? v : 0) }
{
    trace (TRACE_CREATE, "SM:%p created (%s:%#lx)", static_cast<void *>(this), p ? "GSI" : "CNT", v);
}
