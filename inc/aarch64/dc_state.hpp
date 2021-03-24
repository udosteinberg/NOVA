/*
 * Device Context (DC) State
 *
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

#include "gits.hpp"
#include "smmu.hpp"

class Dc_state
{
    public:
        uint32_t const did;
        uint32_t const sid;
        Smmu *   const smmu;
        Gits *   const gits;
        void *   const itt;
        uint8_t  const smg;
        uint8_t  const ctx;

    protected:
        explicit Dc_state (uint64_t t, uint64_t d, uint64_t i) : did { static_cast<uint32_t>(t >> 32) }, sid { static_cast<uint32_t>(t) }, smmu { Smmu::lookup_phys (d & ~OFFS_MASK (0)) }, gits { Gits::lookup (i & ~OFFS_MASK (0)) }, itt { gits ? gits->itt_alloc (did) : nullptr }, smg { static_cast<uint8_t>(d) }, ctx { static_cast<uint8_t>(i) } {}

        ~Dc_state()
        {
            if (gits)
                gits->itt_free (did, itt);
        }
};
