/*
 * Memory Type Range Registers (MTRR)
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

#include "macros.hpp"
#include "msr.hpp"

class Mtrr final
{
    public:
        static auto get_vcnt() { return static_cast<unsigned>(Msr::read (Msr::Reg64::IA32_MTRR_CAP) & BIT_RANGE (7, 0)); }

        static auto get_base (unsigned n) { return Msr::read (Msr::Arr64::IA32_MTRR_PHYS_BASE, 2, n); }
        static auto get_mask (unsigned n) { return Msr::read (Msr::Arr64::IA32_MTRR_PHYS_MASK, 2, n); }

        static void set_base (unsigned n, uint64_t v) { Msr::write (Msr::Arr64::IA32_MTRR_PHYS_BASE, 2, n, v); }
        static void set_mask (unsigned n, uint64_t v) { Msr::write (Msr::Arr64::IA32_MTRR_PHYS_MASK, 2, n, v); }

        static bool validate (unsigned n, uint64_t m)
        {
            for (unsigned i { 0 }; i < n; i++)
                if (get_mask (i) & m)
                    return false;

            return true;
        }
};
