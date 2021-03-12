/*
 * DMA Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "ptab_dpt_amd.hpp"
#include "ptab_dpt_itl.hpp"
#include "sdid.hpp"
#include "smmu.hpp"
#include "space_mem.hpp"

class Space_dma : public Space_mem<Space_dma>
{
    private:
        uint16_t const  sdid;
        Dptp_amd        dptp_amd;
        Dptp_itl        dptp_itl;

    public:
        static Space_dma nova;

        // Constructor
        explicit Space_dma() : sdid { Sdid::allocator.alloc().val() } {}

        // Destructor
        ~Space_dma() { Sdid::allocator.free (sdid); }

        [[nodiscard]] auto get_sdid() const { return sdid; }
        [[nodiscard]] auto get_root_amd (unsigned l) { return dptp_amd.root_init (l - 1); }
        [[nodiscard]] auto get_root_itl (unsigned l) { return dptp_itl.root_init (l - 1); }

        static uint8_t sbw()
        {
            switch (Smmu::type()) {
                case Smmu::Type::AMD: return Dpt_amd::ibits - PAGE_BITS;
                case Smmu::Type::ITL: return Dpt_itl::ibits - PAGE_BITS;
                default: return 0;
            }
        }

        static uint8_t mco()
        {
            switch (Smmu::type()) {
                case Smmu::Type::AMD: return static_cast<uint8_t>(Dpt_amd::lev_ord());
                case Smmu::Type::ITL: return static_cast<uint8_t>(Dpt_itl::lev_ord());
                default: return 0;
            }
        }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma)
        {
            switch (Smmu::type()) {
                case Smmu::Type::AMD: return dptp_amd.update (v, p, o, pm, ma);
                case Smmu::Type::ITL: return dptp_itl.update (v, p, o, pm, ma);
                default: return Status::BAD_FTR;
            }
        }

        auto sync() const { return Smmu::all_invalidate_tlb (sdid); }

        static void access_ctrl (uint64_t phys, size_t size, Paging::Permissions perm) { Space_mem::access_ctrl (nova, phys, size, perm, Memattr::ram()); }
};
