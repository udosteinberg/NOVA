/*
 * DMA Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

class Space_dma : public Space_mem<Space_dma>
{
    private:
        Dptp    dptp;
        Sdid    sdid;

    public:
        static Space_dma nova;

        static constexpr auto num { BIT64 (Dptp::lev * Dptp::bpl) };

        [[nodiscard]] inline auto get_ptab (unsigned l) { return dptp.root_init (Smmu::nc, l); }

        inline auto update (uint64 v, uint64 p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh) { return dptp.update (v, p, o, pm, ca, sh, Smmu::nc); }

        inline void sync() { Smmu::invalidate_all (sdid); }

        inline auto get_sdid() const { return sdid; }

        static void user_access (uint64 addr, size_t size, bool a) { Space_mem::user_access (nova, addr, size, a, Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE); }
};
