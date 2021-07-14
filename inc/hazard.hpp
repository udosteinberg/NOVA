/*
 * Hazard
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
#include "macros.hpp"

class Hazard final
{
    private:
        using hazard_t = unsigned;

        /*
         * CPU A (setting hazard for EC X)              CPU B (switching to EC X)
         *
         * (1) ST.SEQ_CST (X->hazard = RECALL)          (3) ST.SEQ_CST (Ec::current = X)
         * (2) LD.SEQ_CST (Ec::current)                 (4) LD.SEQ_CST (X->hazard)
         *
         * Required Memory Ordering
         *
         * (1) happens before (2)                       (3) happens before (4)
         * (2) synchronizes with (3)                    (4) synchronizes with (1)
         */
        Atomic<hazard_t, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST> val;

    public:
        enum
        {
            SCHED       = BIT  (0),
            SLEEP       = BIT  (1),
            RCU         = BIT  (2),
            TR          = BIT (15),     // x86 only
            FPU         = BIT (16),
            TSC         = BIT (29),     // x86 only
            RECALL      = BIT (30),
            ILLEGAL     = BIT (31),
        };

        explicit constexpr Hazard (hazard_t h) : val { h } {}

        operator hazard_t() const { return val; }

        void set (hazard_t h) { val |=  h; }
        void clr (hazard_t h) { val &= ~h; }
        auto tas (hazard_t h) { return val.test_and_set (h); }
};
