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
#include "pcid.hpp"
#include "ptab_tmp.hpp"
#include "sdid.hpp"

class Space_mem
{
    private:
        uint16_t const pcid;
        uint16_t const sdid;

    public:
        Hptp loc[NUM_CPU];
        Hptp hpt;
        union {
            Dptp_amd dpt_amd;
            Dptp_itl dpt_itl;
        };
        union {
            Eptp ept;
            Hptp npt;
        };

        Cpuset cpus;
        Cpuset htlb;
        Cpuset gtlb;

        explicit Space_mem() : pcid { Pcid::allocator.alloc().val() }, sdid { Sdid::allocator.alloc().val() } {}

        ~Space_mem() { Pcid::allocator.free (pcid); Sdid::allocator.free (sdid); }

        auto get_pcid() const { return pcid; }
        auto get_sdid() const { return sdid; }

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
