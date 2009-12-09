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

#pragma once

#include "compiler.h"
#include "config.h"
#include "cpu.h"
#include "dpt.h"
#include "ept.h"
#include "ptab.h"
#include "types.h"
#include "vma.h"

class Vma;

class Space_mem
{
    protected:
        Vma     vma_head;
        Dpt *   d;
        Ept *   e;
        Ptab *  master;
        Ptab *  percpu[NUM_CPU];

    public:
        // Constructor for kernel memory space
        ALWAYS_INLINE INIT
        inline Space_mem() : d (new Dpt), master (Ptab::master()) {}

        // Constructor
        Space_mem (bool);

        ALWAYS_INLINE
        inline Dpt *dpt() const { return d; }

        ALWAYS_INLINE
        inline Ept *ept() const { return e; }

        ALWAYS_INLINE
        inline Ptab *mst_ptab() const
        {
            return master;
        }

        ALWAYS_INLINE
        inline Ptab *cpu_ptab (unsigned cpu) const
        {
            return percpu[cpu];
        }

        ALWAYS_INLINE
        inline void insert_local (mword virt, unsigned o, Paging::Attribute attr, Paddr phys)
        {
            percpu[Cpu::id]->insert (virt, o, attr, phys);
        }

        ALWAYS_INLINE
        inline bool lookup (mword virt, size_t &size, Paddr &phys)
        {
            return master->lookup (virt, size, phys);
        }

        ALWAYS_INLINE
        inline void insert (mword virt, unsigned o, Paging::Attribute attr, Paddr phys)
        {
            master->insert (virt, o, attr, phys);
        }

        INIT
        void insert_root (mword, size_t, unsigned, unsigned);

        bool insert (Vma *, Paddr);

        void init (unsigned);

        ALWAYS_INLINE
        inline bool sync (mword hla)
        {
            return percpu[Cpu::id]->sync_from (master, hla);
        }

        static void page_fault (mword, mword);
};
