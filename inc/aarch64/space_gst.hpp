/*
 * Guest Memory Space
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

class Space_gst final : public Space_mem<Space_gst>
{
    private:
        uint16_t const  vmid;
        Nptp            nptp;

        // Constructor
        explicit Space_gst (Refptr<Pd> &ref_pd, uint16_t v) : Space_mem { Kobject::Subtype::GST, ref_pd }, vmid { v } {}

        // Destructor
        ~Space_gst() { Vmid::allocator.free (vmid); }

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: GST %p collected", static_cast<void *>(this));
        }

    public:
        static constexpr uint8_t sbw() { return Npt::ibits - PAGE_BITS; }
        static           uint8_t mco() { return static_cast<uint8_t>(Npt::lev_ord()); }

        [[nodiscard]] static Space_gst *create (Status &, Pd *);

        void destroy() override final;

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma, bool &inv) { return nptp.update (v, p, o, pm, ma, inv); }

        [[nodiscard]] auto sync() { nptp.invalidate (vmid); return true; }

        void make_current() { nptp.make_current (vmid); }
};
