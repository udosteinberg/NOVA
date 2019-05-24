/*
 * Portal
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "kobject.hpp"
#include "mtd.hpp"

class Ec;

class Pt : public Kobject
{
    friend class Ec;

    private:
        Ec *        const       ec                  { nullptr };
        mword       const       ip                  { 0 };
        mword                   id                  { 0 };
        Mtd_arch                mtd                 { 0 };

        static Slab_cache       cache;

        Pt (Ec *, mword);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }

    public:
        static Pt *create (Ec *e, mword i)
        {
            Pt *ptr (new Pt (e, i));
            return ptr;
        }

        void destroy() { delete this; }

        ALWAYS_INLINE
        inline void set_id (mword i) { id = i; }

        ALWAYS_INLINE
        inline void set_mtd (Mtd_arch m) { mtd = m; }

};
