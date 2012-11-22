/*
 * Portal
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

#include "ec.h"
#include "pt.h"
#include "stdio.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pt::cache (sizeof (Pt), 32);

Pt::Pt (Pd *own, mword sel, Ec *e, Mtd m, mword addr) : Kobject (PT, static_cast<Space_obj *>(own), sel, 0x1), ec (e), mtd (m), ip (addr)
{
    trace (TRACE_SYSCALL, "PT:%p created (EC:%p IP:%#lx)", this, e, ip);
}
