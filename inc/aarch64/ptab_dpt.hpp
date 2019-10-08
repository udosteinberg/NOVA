/*
 * DMA Page Table (DPT)
 *
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

#include "ptab_npt.hpp"
#include "smmu.hpp"

class Dptp final : public Pagetable<Npt, uint64, uint64, 3, 3, true>
{
    public:
        // Constructor
        ALWAYS_INLINE
        inline explicit Dptp (OAddr v = 0) : Pagetable (Npt (v)) {}

        ALWAYS_INLINE
        inline void invalidate (Vmid v) const { Smmu::invalidate_all (v); }
};
