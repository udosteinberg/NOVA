/*
 * Guest Memory Space
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

        static constexpr uint8_t sbw() { return Ept::ibits - PAGE_BITS; }
        static           uint8_t mco() { return static_cast<uint8_t>(Ept::lev_ord()); }

        auto lookup (uint64_t v, uint64_t &p, unsigned &o, Memattr &ma) const { return eptp.lookup (v, p, o, ma); }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return eptp.update (v, p, o, pm, ma); }

        void sync() { gtlb.set_all(); Tlb::shootdown (this); }

        auto invalidate() const { return eptp.invalidate(); }

        auto get_phys() const { return eptp.root_addr(); }
};
