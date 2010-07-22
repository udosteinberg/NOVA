/*
 * Memory Space
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "cpuset.h"
#include "dpt.h"
#include "ept.h"
#include "ptab.h"
#include "space.h"

class Space_mem : public Space
{
    protected:
        Ptab *  percpu[NUM_CPU];

    public:
        Ptab *const mst;
        mword did;
        Dpt   dpt;
        Ept   ept;

        Cpuset cpus;
        Cpuset htlb;
        Cpuset gtlb;

        static unsigned did_ctr;

        ALWAYS_INLINE
        inline Space_mem (Ptab *p) : mst (p) {}

        ALWAYS_INLINE
        inline Ptab *cpu_ptab (unsigned cpu) const
        {
            assert (cpu < NUM_CPU);
            return percpu[cpu];
        }

        ALWAYS_INLINE
        inline void insert_local (mword virt, unsigned o, Paging::Attribute attr, Paddr phys)
        {
            percpu[Cpu::id]->update (virt, o, phys, attr);
        }

        ALWAYS_INLINE
        inline size_t lookup (mword virt, Paddr &phys)
        {
            return mst->lookup (virt, phys);
        }

        ALWAYS_INLINE
        inline void insert (mword virt, unsigned o, Paging::Attribute attr, Paddr phys)
        {
            mst->update (virt, o, phys, attr);
        }

        ALWAYS_INLINE
        inline Paddr lookup_obj (mword addr, bool priv)
        {
            Paddr phys;

            addr <<= PAGE_BITS;

            if (priv)
                phys = addr;
            else {
                size_t size = lookup (addr, phys);
                assert (size);
            }

            return phys;
        }

        INIT
        void insert_root (mword, size_t, mword);

        bool insert_utcb (mword);

        void update (Mdb *, Paddr, mword, mword);

        static void shootdown();

        void init (unsigned);

        ALWAYS_INLINE
        inline bool sync_mst (mword hla)
        {
            return percpu[Cpu::id]->sync_from (mst, hla);
        }

        ALWAYS_INLINE
        inline bool sync_glb (mword hla)
        {
            return percpu[Cpu::id]->sync_from (Ptab::master(), hla);
        }
};
