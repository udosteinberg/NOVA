/*
 * Hazard
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

#include "atomic.hpp"
#include "macros.hpp"

class Hazard final
{
    private:
        using hazard_t = unsigned;

        Atomic<hazard_t> val;

    public:
        enum
        {
            SCHED       = BIT  (0),
            SLEEP       = BIT  (1),
            RCU         = BIT  (2),
            BOOT_HST    = BIT  (8),
            BOOT_GST    = BIT  (9),
            TR          = BIT (15),     // x86 only
            FPU         = BIT (16),
            TSC         = BIT (29),     // x86 only
            RECALL      = BIT (30),
            ILLEGAL     = BIT (31),
        };

        inline Hazard (hazard_t h) : val (h) {}

        inline operator hazard_t() const { return val; }

        inline void set (hazard_t h) { val |=  h; }
        inline void clr (hazard_t h) { val &= ~h; }
        inline auto tas (hazard_t h) { return val.test_and_set (h); }
};
