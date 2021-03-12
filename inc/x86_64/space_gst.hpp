/*
 * Guest Memory Space
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

#include "cpuset.hpp"
#include "ptab_ept.hpp"
#include "space_mem.hpp"
#include "tlb.hpp"

class Space_gst final : public Space_mem<Space_gst>
{
    private:
        Eptp    eptp;

        inline Space_gst (Pd *p) : Space_mem (Kobject::Subtype::GST, p) {}

    public:
        Cpuset  gtlb;

        static inline auto selectors() { return BIT64 (Ept::ibits - PAGE_BITS); }
        static inline auto max_order() { return Ept::lev_ord(); }

        [[nodiscard]] static inline Space_gst *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            auto const gst { new (cache) Space_gst (pd) };

            if (EXPECT_TRUE (gst)) {

                if (EXPECT_TRUE (gst->eptp.root_init()))
                    return gst;

                operator delete (gst, cache);
            }

            s = Status::MEM_OBJ;

            return nullptr;
        }

        inline void destroy (Slab_cache &cache) { operator delete (this, cache); }

        inline auto lookup (uint64_t v, uint64_t &p, unsigned &o, Memattr &ma) const { return eptp.lookup (v, p, o, ma); }

        inline auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return eptp.update (v, p, o, pm, ma); }

        inline void sync() { gtlb.set(); Tlb::shootdown (this); }

        inline void invalidate() { eptp.invalidate(); }

        inline auto get_phys() const { return eptp.root_addr(); }
};
