/*
 * Host Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "cpu.hpp"
#include "cpuset.hpp"
#include "pcid.hpp"
#include "ptab_hpt.hpp"
#include "space_mem.hpp"

class Space_hst : public Space_mem<Space_hst>
{
    public:
        Pcid const  pcid;
        Hptp        hptp;

        Hptp        loc[NUM_CPU];
        Cpuset      cpus;
        Cpuset      htlb;

        static Space_hst nova;
        static Space_hst *current CPULOCAL;

        static inline auto selectors() { return BIT64 (Hpt::ibits - PAGE_BITS - 1); }
        static inline auto max_order() { return Hpt::lev_ord(); }

        [[nodiscard]] inline auto get_ptab (unsigned cpu) { return loc[cpu].root_init(); }

        inline auto lookup (uint64_t v, uint64_t &p, unsigned &o, Memattr &ma) const { return hptp.lookup (v, p, o, ma); }

        inline auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return hptp.update (v, p, o, pm, ma); }

        inline void sync() { htlb.set(); }

        ALWAYS_INLINE
        inline void make_current()
        {
            uintptr_t p = pcid;

            if (EXPECT_FALSE (htlb.tst (Cpu::id)))
                htlb.clr (Cpu::id);

            else {

                if (EXPECT_TRUE (current == this))
                    return;

                p |= BIT64 (63);
            }

            current = this;

            loc[Cpu::id].make_current (Cpu::feature (Cpu::Feature::PCID) ? p : 0);
        }

        inline auto get_pcid() const { return pcid; }

        void init (unsigned);

        static void user_access (uint64_t addr, size_t size, bool a) { Space_mem::user_access (nova, addr, size, a, Memattr::dev()); }
};
