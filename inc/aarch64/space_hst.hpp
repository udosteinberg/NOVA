/*
 * Host Memory Space
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "ptab_npt.hpp"
#include "space_mem.hpp"

class Space_hst final : public Space_mem<Space_hst>
{
    private:
        uint16_t const  vmid;
        Nptp            nptp;

        explicit Space_hst();

        // Constructor
        explicit Space_hst (Refptr<Pd> &ref_pd, uint16_t v) : Space_mem { Kobject::Subtype::HST, ref_pd }, vmid { v } {}

        // Destructor
        ~Space_hst() { Vmid::allocator.free (vmid); }

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: HST %p collected", static_cast<void *>(this));
        }

    public:
        static Space_hst nova;

        static constexpr uint8_t sbw() { return Npt::ibits - PAGE_BITS; }
        static           uint8_t mco() { return static_cast<uint8_t>(Npt::lev_ord()); }

        [[nodiscard]] static Space_hst *create (Status &, Pd *);

        void destroy() override final;

        auto lookup (uint64_t v, uint64_t &p, unsigned &o, Memattr &ma) const { return nptp.lookup (v, p, o, ma); }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma)            { return nptp.update (v, p, o, pm, ma); }
        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma, bool &inv) { return nptp.update (v, p, o, pm, ma, inv); }

        [[nodiscard]] auto sync() { nptp.invalidate (vmid); return true; }

        void make_current() { nptp.make_current (vmid); }

        static void access_ctrl (uint64_t phys, size_t size, Paging::Permissions perm) { Space_mem::access_ctrl (nova, phys, size, perm, Memattr::dev()); }
};
