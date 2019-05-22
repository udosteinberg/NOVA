/*
 * Object Space
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

#include "bits.hpp"
#include "capability.hpp"
#include "macros.hpp"
#include "memory.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_obj : public Space
{
    private:
        struct Captable;

        static constexpr unsigned lev = 2;
        static constexpr unsigned bpl = bit_scan_reverse (PAGE_SIZE / sizeof (Capability));

        Captable *root { nullptr };

        Capability *walk (unsigned long, bool);

    public:
        static constexpr unsigned long num = BIT64 (lev * bpl);

        ~Space_obj();

        Capability lookup (unsigned long) const;
        Status     update (unsigned long, Capability, Capability &);
        Status     insert (unsigned long, Capability);
};
