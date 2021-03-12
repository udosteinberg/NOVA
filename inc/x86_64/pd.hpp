/*
 * Protection Domain (PD)
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
#include "kmem.hpp"
#include "slab.hpp"
#include "space_mem.hpp"
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"

class Pd : public Kobject, public Space_obj, public Space_mem, public Space_pio, public Space_msr
{
    private:
        static Slab_cache cache;

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    protected:
        Pd();

    public:
        static Atomic<Pd *> current CPULOCAL;
        static inline Pd *  root    { nullptr };

        NODISCARD ALWAYS_INLINE
        static inline Pd *create() { return new Pd; }

        void destroy() { delete this; }

        ALWAYS_INLINE
        static inline Pd *remote_current (unsigned cpu)
        {
            return *Kmem::loc_to_glob (&current, cpu);
        }

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

        inline auto delegate_obj (Pd *pd, unsigned long src, unsigned long dst, unsigned ord, unsigned pmm)                                                                      { return Space_obj::delegate (pd, src, dst, ord, pmm); }
        inline auto delegate_mem (Pd *pd, unsigned long src, unsigned long dst, unsigned ord, unsigned pmm, Space::Index si, Memattr::Cacheability ca, Memattr::Shareability sh) { return Space_mem::delegate (pd, src, dst, ord, pmm, si, ca, sh); }
        inline auto delegate_pio (Pd *pd, unsigned long src, unsigned long dst, unsigned ord, unsigned pmm, Space::Index si)                                                     { return Space_pio::delegate (pd, src, dst, ord, pmm, si); }
        inline auto delegate_msr (Pd *pd, unsigned long src, unsigned long dst, unsigned ord, unsigned pmm, Space::Index si)                                                     { return Space_msr::delegate (pd, src, dst, ord, pmm, si); }
};
