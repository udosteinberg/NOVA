/*
 * Memory Space
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

#include "compiler.hpp"
#include "ptab_dpt.hpp"
#include "ptab_hpt.hpp"
#include "ptab_npt.hpp"
#include "space.hpp"
#include "status.hpp"
#include "types.hpp"

class Space_mem : private Space
{
    private:
        Vmid    id_hst;
        Vmid    id_gst;
        Nptp    cpu_hst;
        Nptp    cpu_gst;
        Dptp    dma_hst;
        Dptp    dma_gst;

        void sync (Space::Index);

        Paging::Permissions lookup (uint64 v, uint64 &p, unsigned &o, Memattr::Cacheability &ca, Memattr::Shareability &sh) const { return cpu_hst.lookup (v, p, o, ca, sh); }

    public:
        static constexpr unsigned long num { BIT64 (Nptp::lev * Nptp::bpl) };

        inline auto vmid_hst() const { return id_hst; }
        inline auto vmid_gst() const { return id_gst; }

        [[nodiscard]] inline auto rcpu_hst() { return cpu_hst.root_init (false); }
        [[nodiscard]] inline auto rcpu_gst() { return cpu_gst.root_init (false); }
        [[nodiscard]] inline auto rdma_hst() { return dma_hst.root_init (Smmu::nc); }
        [[nodiscard]] inline auto rdma_gst() { return dma_gst.root_init (Smmu::nc); }

        ALWAYS_INLINE inline void make_current_hst() { cpu_hst.make_current (id_hst); }
        ALWAYS_INLINE inline void make_current_gst() { cpu_gst.make_current (id_gst); }

        Status update (uint64, uint64, unsigned, Paging::Permissions, Memattr::Cacheability, Memattr::Shareability, Space::Index = Space::Index::CPU_HST);

        Status delegate (Space_mem const *, unsigned long, unsigned long, unsigned, unsigned, Space::Index, Memattr::Cacheability, Memattr::Shareability);
};
