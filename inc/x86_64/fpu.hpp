/*
 * Floating Point Unit (FPU)
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

#include "arch.hpp"
#include "cpu.hpp"
#include "cr.hpp"
#include "hazards.hpp"
#include "slab.hpp"

class Fpu
{
    private:
        char data[512];

    public:
        ALWAYS_INLINE
        inline void load() const { asm volatile ("fxrstor64 %0" : : "m" (*this)); }

        ALWAYS_INLINE
        inline void save() { asm volatile ("fxsave64 %0" : "=m" (*this)); }

        ALWAYS_INLINE
        static inline void disable()
        {
            Cr::set_cr0 (Cr::get_cr0() | CR0_TS);

            Cpu::hazard &= ~HZD_FPU;
        }

        ALWAYS_INLINE
        static inline void enable()
        {
            asm volatile ("clts");

            Cpu::hazard |= HZD_FPU;
        }

        [[nodiscard]] ALWAYS_INLINE
        static inline void *operator new (size_t, Slab_cache &cache) noexcept
        {
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr, Slab_cache &cache)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

        static void init();
        static void fini();
};
