/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "dpt.hpp"
#include "hpt.hpp"
#include "npt.hpp"
#include "space.hpp"
#include "types.hpp"

class Space_mem : public Space
{
    private:
        Vmid    id_hst;
        Vmid    id_gst;
        Nptp    cpu_hst;
        Nptp    cpu_gst;
        Dptp    dma_hst;
        Dptp    dma_gst;

    public:
        ALWAYS_INLINE
        inline Space_mem()
        {
            cpu_hst.init_root (false);
            cpu_gst.init_root (false);
        }

        inline auto vmid_hst() const { return id_hst; }
        inline auto vmid_gst() const { return id_gst; }

        inline auto ptab_hst() { return dma_hst.init_root (false); }
        inline auto ptab_gst() { return dma_gst.init_root (false); }

        inline void make_current_hst() { cpu_hst.make_current (id_hst); }
        inline void make_current_gst() { cpu_gst.make_current (id_gst); }

        Paging::Permissions lookup (uint64 v, uint64 &p, unsigned &o, Memattr::Cacheability &ca, Memattr::Shareability &sh) { return cpu_hst.lookup (v, p, o, ca, sh); }

        void update (uint64, uint64, unsigned, Paging::Permissions, Memattr::Cacheability, Memattr::Shareability, Space::Index = Space::Index::CPU_HST);

        void flush (Space::Index);
};
