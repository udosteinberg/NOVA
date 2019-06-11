/*
 * Virtual-Machine Identifier (VMID)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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
#include "types.hpp"

class Vmid
{
    private:
        mword val;

        static Vmid current;

        Vmid (mword v) : val (v) {}

        void allocate()
        {
            val = Atomic::add (current.val, 1UL);

            // XXX: Flush TLB on overflow
        }

    public:
        Vmid() { allocate(); }

        mword vmid() const { return val & 0xff; }
};
