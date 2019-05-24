/*
 * Portal (PT)
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

#include "atomic.hpp"
#include "kobject.hpp"
#include "mtd_arch.hpp"
#include "slab.hpp"

class Ec;

class Pt : private Kobject
{
    friend class Ec;

    private:
        Ec *        const   ec      { nullptr };
        uintptr_t   const   ip      { 0 };
        Atomic<uintptr_t>   id      { 0 };
        Atomic<Mtd_arch>    mtd     { 0 };

        static Slab_cache   cache;

        Pt (Ec *, uintptr_t);

        NODISCARD
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    public:
        NODISCARD
        static Pt *create (Ec *e, uintptr_t i)
        {
            return new Pt (e, i);
        }

        void destroy() { delete this; }

        ALWAYS_INLINE
        inline uintptr_t get_id() const { return id; }

        ALWAYS_INLINE
        inline Mtd_arch get_mtd() const { return mtd; }

        ALWAYS_INLINE
        inline void set_id (uintptr_t i) { id = i; }

        ALWAYS_INLINE
        inline void set_mtd (Mtd_arch m) { mtd = m; }
};
