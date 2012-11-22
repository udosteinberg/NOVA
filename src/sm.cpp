/*
 * Semaphore
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "sm.h"
#include "stdio.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Sm::cache (sizeof (Sm), 32);

Sm::Sm (Pd *own, mword sel, mword cnt) : Kobject (SM, static_cast<Space_obj *>(own), sel, 0x3), counter (cnt)
{
    trace (TRACE_SYSCALL, "SM:%p created (CNT:%lu)", this, cnt);
}
