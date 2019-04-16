/*
 * Protection Domain (PD)
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
#include "kobject.hpp"
#include "slab.hpp"
#include "space_mem.hpp"
#include "space_obj.hpp"

class Pd : public Kobject, public Space_mem, public Space_obj
{
    private:
        static Slab_cache cache;

        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    protected:
        Pd();

    public:
        static Pd root;

        NODISCARD
        static Pd *create()
        {
            return new Pd;
        }

        void destroy() { delete this; }

        bool update_space_obj (Pd *, mword, mword, unsigned, unsigned);
        bool update_space_mem (Pd *, mword, mword, unsigned, unsigned, Memattr::Cacheability, Memattr::Shareability, Space::Index);
        bool update_space_pio (Pd *, mword, mword, unsigned, unsigned) { return false; }
};
