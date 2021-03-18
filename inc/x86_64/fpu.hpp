/*
 * Floating Point Unit (FPU)
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

#include "arch.hpp"
#include "cpu.hpp"
#include "hazards.hpp"
#include "lowlevel.hpp"
#include "slab.hpp"

class Fpu
{
    private:
        char data[512];

        static Slab_cache cache;

    public:
        ALWAYS_INLINE
        inline void save() { asm volatile ("fxsave %0" : "=m" (*data)); }

        ALWAYS_INLINE
        inline void load() { asm volatile ("fxrstor %0" : : "m" (*data)); }

        ALWAYS_INLINE
        static inline void init() { asm volatile ("fninit"); }

        ALWAYS_INLINE
        static inline void enable() { asm volatile ("clts"); Cpu::hazard |= HZD_FPU; }

        ALWAYS_INLINE
        static inline void disable() { set_cr0 (get_cr0() | CR0_TS); Cpu::hazard &= ~HZD_FPU; }

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
};
