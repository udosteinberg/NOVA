/*
 * Guest Memory Space
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

#include "cpuset.hpp"
#include "ptab_ept.hpp"
#include "space_mem.hpp"
#include "tlb.hpp"

class Space_gst : public Space_mem<Space_gst>
{
    private:
        Eptp    eptp;

    public:
        Cpuset  gtlb;

        static constexpr auto num { BIT64 (Eptp::lev * Eptp::bpl) };

        inline auto update (uint64 v, uint64 p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh) { return eptp.update (v, p, o, pm, ca, sh); }

        inline void sync() { gtlb.set(); Tlb::shootdown (this); }

        inline void invalidate() { eptp.invalidate(); }

        inline auto get_phys() const { return eptp.root_addr(); }
};
