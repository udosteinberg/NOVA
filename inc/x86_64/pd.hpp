/*
 * Protection Domain
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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
#include "kmem.hpp"
#include "slab.hpp"
#include "space_mem.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"

class Pd : public Kobject, public Space_mem, public Space_pio, public Space_obj
{
    private:
        static Slab_cache cache;

    public:
        static Atomic<Pd *> current CPULOCAL;
        static Pd kern, root;

        Pd (Pd *);

        Pd (Pd *, mword, mword) : Kobject (Kobject::Type::PD) {}

        ALWAYS_INLINE HOT
        inline void make_current()
        {
            uintptr_t p = pcid();

            if (EXPECT_FALSE (htlb.chk (Cpu::id)))
                htlb.clr (Cpu::id);

            else {

                if (EXPECT_TRUE (current == this))
                    return;

                p |= BIT64 (63);
            }

            current = this;

            loc[Cpu::id].make_current (Cpu::feature (Cpu::Feature::PCID) ? p : 0);
        }

        ALWAYS_INLINE
        static inline Pd *remote (unsigned c)
        {
            return *Kmem::loc_to_glob (&current, c);
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
