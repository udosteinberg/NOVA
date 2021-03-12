/*
 * Host Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

        Space_hst (Refptr<Pd> &p) : Space_mem { Kobject::Subtype::HST, p } {}

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: HST %p collected", static_cast<void *>(this));
        }

    public:
        Pcid const  pcid;
        Hptp        hptp;

        Hptp        loc[NUM_CPU];
        Cpuset      cpus;
        Cpuset      htlb;

        static Space_hst nova;
        static Space_hst *current CPULOCAL;

        static constexpr uint8_t  sbw       { Hpt::ibits - PAGE_BITS - 1 };
        static constexpr uint64_t selectors { BIT64 (sbw) };

        static auto mco() { return static_cast<uint8_t>(Hpt::lev_ord()); }

        [[nodiscard]] auto get_ptab (unsigned cpu) { return loc[cpu].root_init(); }

        [[nodiscard]] static Space_hst *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            // Acquire reference
            Refptr<Pd> ref_pd { pd };

            // Failed to acquire reference
            if (!ref_pd) [[unlikely]]
                s = Status::ABORTED;

            else {

                auto const hst { new (cache) Space_hst { ref_pd } };

                // If we created hst, then reference must have been consumed
                assert (!hst || !ref_pd);

                if (hst) [[likely]] {

                    if (hst->hptp.root_init()) [[likely]]
                        return hst;

                    operator delete (hst, cache);
                }

                s = Status::MEM_OBJ;
            }

            return nullptr;
        }

        void destroy()
        {
            auto &cache { get_pd()->hst_cache };

            this->~Space_hst();

            operator delete (this, cache);
        }

        auto lookup (uint64_t v, uint64_t &p, unsigned &o, Memattr &ma) const { return hptp.lookup (v, p, o, ma); }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return hptp.update (v, p, o, pm, ma); }

        void sync() { htlb.set(); Tlb::shootdown (this); }

        ALWAYS_INLINE
        inline void make_current()
        {
            uintptr_t p = pcid;

            if (htlb.tst (Cpu::id)) [[unlikely]]
                htlb.clr (Cpu::id);

            else {

                if (current == this) [[likely]]
                    return;

                p |= BIT64 (63);
            }

            current = this;

            loc[Cpu::id].make_current (Cpu::feature (Cpu::Feature::PCID) ? p : 0);
        }

        auto get_pcid() const { return pcid; }

        void init (cpu_t);

        static void access_ctrl (uint64_t phys, size_t size, Paging::Permissions perm) { Space_mem::access_ctrl (nova, phys, size, perm, Memattr::dev()); }
};
