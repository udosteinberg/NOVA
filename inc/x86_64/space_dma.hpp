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
#include "sdid.hpp"
#include "space_mem.hpp"

class Space_dma : public Space_mem<Space_dma>
{
    private:
        Sdid const  sdid;
        Dptp        dptp;

    public:
        static Space_dma nova;

        static inline auto selectors() { return BIT64 (Dpt::ibits - PAGE_BITS); }
        static inline auto max_order() { return Dptp::lim; }

        [[nodiscard]] inline auto get_ptab (unsigned l) { return dptp.root_init (l); }

        inline auto update (uint64 v, uint64 p, unsigned o, Paging::Permissions pm, Memattr ma) { return dptp.update (v, p, o, pm, ma); }

        inline void sync() {}

        inline auto get_sdid() const { return sdid; }

        static void user_access (uint64 addr, size_t size, bool a) { Space_mem::user_access (nova, addr, size, a, Memattr::ram()); }
};
