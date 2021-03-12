/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "config.hpp"
#include "cpu.hpp"
#include "cpuset.hpp"
#include "dpt.hpp"
#include "ept.hpp"
#include "hpt.hpp"
#include "pcid.hpp"
#include "space.hpp"
#include "vpid.hpp"

class Space_mem : public Space
{
    private:
        Pcid id_hst;
        Vpid id_gst;

    public:
        Hptp loc[NUM_CPU];
        Hptp hpt;
        Dptp dpt;
        union {
            Eptp ept;
            Hptp npt;
        };

        Cpuset cpus;
        Cpuset htlb;
        Cpuset gtlb;

        static constexpr unsigned long num = BIT64 (Hpt::lev * Hpt::bpl - 1);

        ALWAYS_INLINE
        inline Space_mem() {}

        inline auto pcid() const { return id_hst; }
        inline auto vpid() const { return id_gst; }

        Paging::Permissions lookup (uint64 v, uint64 &p, unsigned &o, Memattr::Cacheability &ca, Memattr::Shareability &sh) { return hpt.lookup (v, p, o, ca, sh); }

        void update (mword, mword, unsigned, Paging::Permissions, Memattr::Cacheability, Memattr::Shareability, Space::Index = Space::Index::CPU_HST);

        void flush (Space::Index);

        static void shootdown();

        void init (unsigned);
};
