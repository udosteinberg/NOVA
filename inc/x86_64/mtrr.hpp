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

#include "msr.hpp"

class Mtrr final
{
    public:
        static auto get_vcnt() { return static_cast<unsigned>(Msr::read (Msr::Register::IA32_MTRR_CAP) & BIT_RANGE (7, 0)); }

        static auto get_base (unsigned n) { return Msr::read (Msr::Array::IA32_MTRR_PHYS_BASE, 2, n); }
        static auto get_mask (unsigned n) { return Msr::read (Msr::Array::IA32_MTRR_PHYS_MASK, 2, n); }

        static void set_base (unsigned n, uint64_t v) { Msr::write (Msr::Array::IA32_MTRR_PHYS_BASE, 2, n, v); }
        static void set_mask (unsigned n, uint64_t v) { Msr::write (Msr::Array::IA32_MTRR_PHYS_MASK, 2, n, v); }
};
