/*
 * Memory Space
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

#pragma once

#include "compiler.h"
#include "dpt.h"
#include "ept.h"
#include "ptab.h"
#include "space.h"

class Space_mem : public Space
{
    private:
        static unsigned did_ctr;

    protected:
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
        inline size_t lookup (mword virt, Paddr &phys)
        {
            return mst->lookup (virt, phys);
        }

        ALWAYS_INLINE
        inline void insert (mword virt, unsigned o, Paging::Attribute attr, Paddr phys)
        {
            mst->insert (virt, o, attr, phys);
        }

        ALWAYS_INLINE
        inline Paddr lookup_obj (mword addr, bool priv)
        {
            Paddr phys;

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

        static void insert (Mdb *, Paddr);

        void init (unsigned);

        ALWAYS_INLINE
        inline bool sync (mword hla)
        {
            return percpu[Cpu::id]->sync_from (mst, hla);
        }
};
