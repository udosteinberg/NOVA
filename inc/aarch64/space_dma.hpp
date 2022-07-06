/*
 * DMA Memory Space
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

#include "ptab_dpt.hpp"
#include "sdid.hpp"
#include "space_mem.hpp"

class Space_dma final : public Space_mem<Space_dma>
{
    private:
        Sdid const  sdid;
        Dptp        dptp;

        Space_dma (Refptr<Pd> &ref_pd) : Space_mem { Kobject::Subtype::DMA, ref_pd } {}

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: DMA %p collected", static_cast<void *>(this));
        }

    public:
        static constexpr uint8_t  sbw       { Dpt::ibits - PAGE_BITS };
        static constexpr uint64_t selectors { BIT64 (sbw) };

        static auto mco() { return static_cast<uint8_t>(Dpt::lev_ord()); }

        [[nodiscard]] auto get_ptab (unsigned l) { return dptp.root_init (l); }

        [[nodiscard]] static Space_dma *create (Status &, Pd *);

        void destroy() override final;

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return dptp.update (v, p, o, pm, ma); }

        void sync() {}

        auto get_sdid() const { return sdid; }
};
