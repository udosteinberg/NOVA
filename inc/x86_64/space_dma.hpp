/*
 * DMA Memory Space
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

#include "ptab_dpt.hpp"
#include "smmu.hpp"
#include "space_mem.hpp"

class Space_dma : public Space_mem<Space_dma>
{
    private:
        Sdid const  sdid;
        Dptp        dptp;

    public:
        static Space_dma nova;

        static constexpr uint8_t  sbw       { Dpt::ibits - PAGE_BITS };
        static constexpr uint64_t selectors { BIT64 (sbw) };

        static auto mco() { return static_cast<uint8_t>(Dpt::lev_ord()); }

        [[nodiscard]] auto get_ptab (unsigned l) { return dptp.root_init (l); }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return dptp.update (v, p, o, pm, ma); }

        void sync() { Smmu::all_invalidate_tlb (sdid); }

        auto get_sdid() const { return sdid; }

        static void access_ctrl (uint64_t phys, size_t size, Paging::Permissions perm) { Space_mem::access_ctrl (nova, phys, size, perm, Memattr::ram()); }
};
