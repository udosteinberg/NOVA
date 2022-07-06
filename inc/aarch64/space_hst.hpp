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
        Vmid const  vmid;
        Nptp        nptp;

        Space_hst();

        Space_hst (Refptr<Pd> &ref_pd) : Space_mem { Kobject::Subtype::HST, ref_pd } {}

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: HST %p collected", static_cast<void *>(this));
        }

    public:
        static Space_hst nova;

        static constexpr uint8_t  sbw       { Npt::ibits - PAGE_BITS };
        static constexpr uint64_t selectors { BIT64 (sbw) };

        static auto mco() { return static_cast<uint8_t>(Npt::lev_ord()); }

        [[nodiscard]] static Space_hst *create (Status &, Pd *);

        void destroy() override final;

        auto lookup (uint64_t v, uint64_t &p, unsigned &o, Memattr &ma) const { return nptp.lookup (v, p, o, ma); }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return nptp.update (v, p, o, pm, ma); }

        void sync() { nptp.invalidate (vmid); }

        void make_current() { nptp.make_current (vmid); }

        static void access_ctrl (uint64_t phys, size_t size, Paging::Permissions perm) { Space_mem::access_ctrl (nova, phys, size, perm, Memattr::dev()); }
};
