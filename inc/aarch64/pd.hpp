/*
 * Protection Domain (PD)
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

#include "compiler.hpp"
#include "kobject.hpp"
#include "slab.hpp"
#include "space_mem.hpp"
#include "space_obj.hpp"

class Pd : public Kobject, public Space_mem, public Space_obj
{
    private:
        static Slab_cache cache;

        Pd();

        Pd (mword);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }

        static void update_mem_user (Paddr, mword, bool);

    public:
        static Pd kern, root;

        static Pd *create()
        {
            Pd *ptr (new Pd);
            return ptr;
        }

        void destroy() { delete this; }

        static void insert_mem_user (Paddr p, mword s) { update_mem_user (p, s, true);  }
        static void remove_mem_user (Paddr p, mword s) { update_mem_user (p, s, false); }

        void update_obj_space (Pd *, mword, mword, unsigned, unsigned);
        void update_mem_space (Pd *, mword, mword, unsigned, unsigned, Memattr::Cacheability, Memattr::Shareability, Space::Index);
};
