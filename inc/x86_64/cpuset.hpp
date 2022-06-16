/*
 * Atomic CPU Set
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "bits.hpp"
#include "config.hpp"

class Cpuset final
{
    private:
        static constexpr auto cnt { type_bits<uintptr_t>() };
        static constexpr auto idx (cpu_t c) { return c / cnt; }
        static constexpr auto msk (cpu_t c) { return BITN (c % cnt); }

        /*
         * CPU A (switching to EC X)            CPU B (marking TLB dirty for EC X->HST)
         *
         * (1) ST.SEQ_CST (Ec::current = X)     (3) ST.SEQ_CST (X->HST->cpuset = dirty)
         * (2) LD.SEQ_CST (X->HST->cpuset)      (4) LD.SEQ_CST (Ec::current)
         *
         * Required Memory Ordering
         *
         * (1) sequenced-before (2)             (3) sequenced-before (4)
         * (2) synchronizes-with (3)            (4) synchronizes-with (1)
         */
        Atomic<uintptr_t, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST> bitmap[aligned_up (cnt, NUM_CPU) / cnt] {};

    public:
        ALWAYS_INLINE inline void clr (cpu_t c)       {        bitmap[idx (c)] &= ~msk (c); }
        ALWAYS_INLINE inline bool tst (cpu_t c) const { return bitmap[idx (c)] &   msk (c); }
        ALWAYS_INLINE inline bool tas (cpu_t c)       { return bitmap[idx (c)].test_and_set (msk (c)); }
        ALWAYS_INLINE inline void set_all()           { for (auto &n : bitmap) n = ~uintptr_t { 0 }; }
};
