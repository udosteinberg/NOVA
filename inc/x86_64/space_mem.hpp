/*
 * Memory Space
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

#pragma once

#include "config.hpp"
#include "cpu.hpp"
#include "cpuset.hpp"
#include "ptab_dpt.hpp"
#include "ptab_ept.hpp"
#include "ptab_hpt.hpp"

class Space_mem
{
    public:
        Hptp loc[NUM_CPU];
        Hptp hpt;
        Dptp dpt;
        union {
            Eptp ept;
            Hptp npt;
        };

        mword did;

        Cpuset cpus;
        Cpuset htlb;
        Cpuset gtlb;

        static unsigned did_ctr;

        ALWAYS_INLINE
        inline Space_mem() : did (__atomic_add_fetch (&did_ctr, 1, __ATOMIC_SEQ_CST)) {}

        Paging::Permissions lookup (uint64_t v, uint64_t &p, unsigned &o)
        {
            Memattr ma;
            return hpt.lookup (v, p, o, ma);
        }

        void update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma)
        {
            hpt.update (v, p, o, pm, ma);
        }

        void insert_root (uint64_t, uint64_t, uintptr_t = 0x7);

        bool insert_utcb (mword);

        static void shootdown();

        void init (cpu_t);
};
