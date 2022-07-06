/*
 * Guest Memory Space
 *
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

#include "ptab_npt.hpp"
#include "space_mem.hpp"

class Space_gst final : public Space_mem<Space_gst>
{
    private:
        Nptp    nptp;
        Vmid    vmid;

        inline Space_gst (Pd *p) : Space_mem (Kobject::Subtype::GST, p) {}

    public:
        static constexpr auto num { BIT64 (Nptp::lev * Nptp::bpl) };

        [[nodiscard]] static inline Space_gst *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            auto const gst { new (cache) Space_gst (pd) };

            if (EXPECT_TRUE (gst)) {

                if (EXPECT_TRUE (gst->nptp.root_init (false)))
                    return gst;

                operator delete (gst, cache);
            }

            s = Status::INS_MEM;

            return nullptr;
        }

        inline void destroy (Slab_cache &cache) { operator delete (this, cache); }

        inline auto update (uint64 v, uint64 p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh) { return nptp.update (v, p, o, pm, ca, sh); }

        inline void sync() { nptp.invalidate (vmid); }

        inline void make_current() { nptp.make_current (vmid); }
};
