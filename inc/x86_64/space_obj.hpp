/*
 * Object Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "capability.hpp"
#include "macros.hpp"
#include "memory.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_obj : public Space
{
    friend class Pd;

    private:
        struct Captable;

        Atomic<Captable *> root { nullptr };

        static constexpr auto lev { 2 };
        static constexpr auto bpl { bit_scan_reverse (PAGE_SIZE (0) / sizeof (Captable *)) };

        ~Space_obj();

        Atomic<Capability> *walk (unsigned long, bool);

    public:
        static Space_obj nova;

        static constexpr auto selectors { BIT64 (lev * bpl) };
        static constexpr auto max_order { bpl };

        enum Selector
        {
            NOVA_CON = selectors - 1,
            NOVA_OBJ = selectors - 2,
            NOVA_HST = selectors - 3,
            NOVA_PIO = selectors - 4,
            NOVA_MSR = selectors - 5,
            ROOT_OBJ = selectors - 6,
            ROOT_HST = selectors - 7,
            ROOT_PIO = selectors - 8,
            ROOT_PD  = selectors - 9,
            NOVA_INT = BIT (16),
            NOVA_CPU = 0,
        };

        Capability lookup (unsigned long) const;
        Status     update (unsigned long, Capability);
        Status     insert (unsigned long, Capability);

        Status delegate (Space_obj const *, unsigned long, unsigned long, unsigned, unsigned);
};
