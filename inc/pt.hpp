/*
 * Portal (PT)
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

#include "atomic.hpp"
#include "kobject.hpp"
#include "mtd_arch.hpp"
#include "status.hpp"

class Ec;

class Pt final : public Kobject
{
    friend class Ec;

    private:
        Ec *        const   ec      { nullptr };
        uintptr_t   const   ip      { 0 };
        Atomic<uintptr_t>   id      { 0 };
        Atomic<Mtd_arch>    mtd     { Mtd_arch { 0 } };

        static Slab_cache   cache;

        Pt (Ec *, uintptr_t);

    public:
        [[nodiscard]] static inline Pt *create (Status &s, Ec *e, uintptr_t i)
        {
            auto const pt { new (cache) Pt (e, i) };

            if (EXPECT_FALSE (!pt))
                s = Status::MEM_OBJ;

            return pt;
        }

        inline void destroy() { operator delete (this, cache); }

        ALWAYS_INLINE
        inline uintptr_t get_id() const { return id; }

        ALWAYS_INLINE
        inline Mtd_arch get_mtd() const { return mtd; }

        ALWAYS_INLINE
        inline void set_id (uintptr_t i) { id = i; }

        ALWAYS_INLINE
        inline void set_mtd (Mtd_arch m) { mtd = m; }
};
