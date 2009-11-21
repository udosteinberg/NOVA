/*
 * Memory Space
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "memory.h"
#include "pd.h"
#include "regs.h"
#include "space_mem.h"
#include "vma.h"

Space_mem::Space_mem (bool vm)
{
    if (vm)
        ept = new Ept;
}

void Space_mem::init (unsigned cpu)
{
    if (!percpu[cpu]) {

        // Create ptab for this CPU
        percpu[cpu] = new Ptab;

        // Sync cpu-local range
        percpu[cpu]->sync_from (Pd::kern.cpu_ptab (cpu), LOCAL_SADDR);

        // Sync kernel code and data
        percpu[cpu]->sync_master_range (LINK_ADDR, LOCAL_SADDR);

        trace (0, "PD:%p PTAB[%u]:%#lx", static_cast<Pd *>(this), cpu, Buddy::ptr_to_phys (percpu[cpu]));
    }

    if (!master)
        master = percpu[cpu];
}

void Space_mem::page_fault (mword addr, mword /*error*/)
{
    trace (TRACE_MEMORY, "#PF (MEM) PD:%p ADDR:%#lx", Pd::current, addr);

    size_t size; Paddr phys;
    if (Ptab::master()->lookup (addr, size, phys))
        Ptab::current()->sync_master (addr);
}

void Space_mem::insert_root (mword b, size_t s, unsigned t, unsigned a)
{
    for (long int o; s; s -= 1ul << o, b += 1ul << o) {

        o = bit_scan_reverse (s);
        if (b)
            o = min (bit_scan_forward (b), o);

        trace (TRACE_MAP, "MEM B:%#010lx O:%02lu T:%#x A:%#x", b, o, t, a);

        vma_head.create_child (&vma_head, 0, b, o, t, a);
    }
}

bool Space_mem::insert (Vma *vma, Paddr phys)
{
    if (ept)
        ept->insert (vma->base, vma->order - PAGE_BITS, vma->type, vma->attr, phys);

    Ptab::Attribute a = Ptab::Attribute (Ptab::ATTR_USER |
                      (vma->attr & 0x4 ? Ptab::ATTR_NONE     : Ptab::ATTR_NOEXEC) |
                      (vma->attr & 0x2 ? Ptab::ATTR_WRITABLE : Ptab::ATTR_NONE));

    // Whoever owns a VMA struct in the VMA list owns the respective PT slots
    master->insert (vma->base, vma->order - PAGE_BITS, a, phys);

    return true;
}
