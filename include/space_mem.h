/*
 * Memory Space
 *
 * Copyright (C) 2007-2010, Udo Steinberg <udo@hypervisor.org>
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
    private:
        static unsigned did_ctr;

    protected:
        Vma     vma_head;
        Ptab *  percpu[NUM_CPU];

    public:
        Ptab *const mst;
        mword const did;
        Dpt * const dpt;
        Ept * const ept;

        // Constructor for kernel memory space
        ALWAYS_INLINE INIT
        inline Space_mem() : mst (Ptab::master()), did (++did_ctr), dpt (new Dpt), ept (0) {}

        ALWAYS_INLINE
        inline Space_mem (unsigned flags) : mst (new Ptab), did (flags & 0x2 ? ++did_ctr : 0), dpt (flags & 0x2 ? new Dpt : 0), ept (flags & 0x1 ? new Ept : 0) {}

        ALWAYS_INLINE
        inline Ptab *cpu_ptab (unsigned cpu) const
        {
            assert (cpu < NUM_CPU);
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
            return mst->lookup (virt, size, phys);
        }

        ALWAYS_INLINE
        inline void insert (mword virt, unsigned o, Paging::Attribute attr, Paddr phys)
        {
            mst->insert (virt, o, attr, phys);
        }

        INIT
        void insert_root (mword, size_t);

        INIT
        void insert_root (mword, size_t, unsigned, unsigned);

        ALWAYS_INLINE
        inline bool insert_utcb (mword b)
        {
            return !b || vma_head.create_child (&vma_head, 0, b, 0, 0, 0);
        }

        bool insert (Vma *, Paddr);

        void init (unsigned);

        ALWAYS_INLINE
        inline bool sync (mword hla)
        {
            return percpu[Cpu::id]->sync_from (mst, hla);
        }
};
