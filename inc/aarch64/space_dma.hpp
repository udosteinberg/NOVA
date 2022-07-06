/*
 * DMA Memory Space
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

#include "ptab_dpt.hpp"
#include "smmu.hpp"
#include "space_mem.hpp"

class Space_dma final : public Space_mem<Space_dma>
{
    private:
        Dptp    dptp;
        Sdid    sdid;

        inline Space_dma (Pd *p) : Space_mem (Kobject::Subtype::DMA, p) {}

    public:
        static constexpr auto num { BIT64 (Dptp::lev * Dptp::bpl) };

        [[nodiscard]] static inline Space_dma *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            auto const dma { new (cache) Space_dma (pd) };

            if (EXPECT_TRUE (dma)) {

                if (EXPECT_TRUE (dma->dptp.root_init (Smmu::nc)))
                    return dma;

                operator delete (dma, cache);
            }

            s = Status::INS_MEM;

            return nullptr;
        }

        inline void destroy (Slab_cache &cache) { operator delete (this, cache); }

        inline auto update (uint64 v, uint64 p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh) { return dptp.update (v, p, o, pm, ca, sh, Smmu::nc); }

        inline void sync() { Smmu::invalidate_all (sdid); }

        inline auto get_phys() const { return dptp.root_addr(); }

        inline auto get_sdid() const { return sdid; }
};
