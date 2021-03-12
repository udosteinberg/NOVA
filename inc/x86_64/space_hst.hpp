/*
 * Host Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "tlb.hpp"

class Space_hst final : public Space_mem<Space_hst>
{
    private:
        Space_hst();

        inline Space_hst (Pd *p) : Space_mem (Kobject::Subtype::HST, p) {}

    public:
        Hptp    hptp;
        Pcid    pcid;

        Hptp    loc[NUM_CPU];
        Cpuset  cpus;
        Cpuset  htlb;

        static Space_hst nova;
        static Space_hst *current CPULOCAL;

        static constexpr auto num { BIT64 (Hptp::lev * Hptp::bpl - 1) };

        [[nodiscard]] inline auto get_ptab (unsigned cpu) { return loc[cpu].root_init (false); }

        [[nodiscard]] static inline Space_hst *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            auto const hst { new (cache) Space_hst (pd) };

            if (EXPECT_TRUE (hst)) {

                if (EXPECT_TRUE (hst->hptp.root_init (false)))
                    return hst;

                operator delete (hst, cache);
            }

            s = Status::INS_MEM;

            return nullptr;
        }

        inline void destroy (Slab_cache &cache) { operator delete (this, cache); }

        inline auto lookup (uint64 v, uint64 &p, unsigned &o, Memattr::Cacheability &ca, Memattr::Shareability &sh) const { return hptp.lookup (v, p, o, ca, sh); }

        inline auto update (uint64 v, uint64 p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh) { return hptp.update (v, p, o, pm, ca, sh); }

        inline void sync() { htlb.set(); Tlb::shootdown (this); }

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

        static void user_access (uint64 addr, size_t size, bool a) { Space_mem::user_access (nova, addr, size, a, Memattr::Cacheability::MEM_UC, Memattr::Shareability::NONE); }
};
