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
#include "fpu.hpp"
#include "kmem.hpp"
#include "slab.hpp"
#include "space_dma.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"

class Pd : public Kobject, public Space_obj, public Space_hst, public Space_gst, public Space_dma, public Space_pio, public Space_msr
{
    private:
        static Slab_cache cache;

    public:
        Slab_cache fpu_cache;

        static Atomic<Pd *> current CPULOCAL;
        static Pd kern, root;

        Pd (Pd *);

        Pd (Pd *, mword, mword) : Kobject (Kobject::Type::PD), Space_pio (nullptr), Space_msr (nullptr), fpu_cache (sizeof (Fpu), 16) {}

        ALWAYS_INLINE HOT
        inline void make_current()
        {
            uintptr_t p = get_pcid();

            if (EXPECT_FALSE (htlb.tst (Cpu::id)))
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
            return *Kmem::loc_to_glob (c, &current);
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
