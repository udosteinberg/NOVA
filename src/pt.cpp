/*
 * Portal
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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
#include "initprio.h"
#include "pt.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pt::cache (sizeof (Pt), 16);

Pt::Pt (Pd *own, mword sel, Ec *e, Mtd m, mword addr) : Kobject (own, sel, PT), ec (e), mtd (m), ip (addr)
{
    trace (TRACE_SYSCALL, "PT:%p created (EC:%p IP:%#lx)", this, e, ip);
}
