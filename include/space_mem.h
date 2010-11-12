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
#include "hpt.h"
#include "space.h"

class Space_mem : public Space
{
    public:
        Hpt loc[NUM_CPU];
        Hpt hpt;
        Dpt dpt;
        union {
            Ept ept;
            Hpt npt;
        };

        mword did;

        Cpuset cpus;
        Cpuset htlb;
        Cpuset gtlb;

        static unsigned did_ctr;

        ALWAYS_INLINE
        inline size_t lookup (mword virt, Paddr &phys)
        {
            return hpt.lookup (virt, phys);
        }

        ALWAYS_INLINE
        inline void insert (mword virt, unsigned o, mword attr, Paddr phys)
        {
            hpt.update (virt, o, phys, attr);
        }

        INIT
        void insert_root (mword, size_t, mword);

        bool insert_utcb (mword);

        void update (Mdb *, mword = 0);

        static void shootdown();

        void init (unsigned);

        ALWAYS_INLINE
        inline bool sync_mst (mword hla)
        {
            return loc[Cpu::id].sync_from (hpt, hla);
        }

        ALWAYS_INLINE
        inline bool sync_glb (mword hla)
        {
            return loc[Cpu::id].sync_from (Hptp (reinterpret_cast<mword>(&PDBR) + Hpt::HPT_P), hla);
        }
};
