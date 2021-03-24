/*
 * Device Context (DC) State
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
    protected:
        explicit Dc_state (uint64_t t, uint64_t d, uint64_t i) : topo { t }, smmu { Smmu::lookup (d & ~OFFS_MASK (0)) }, gits { Gits::lookup (i & ~OFFS_MASK (0)) }, smg { static_cast<uint8_t>(d) }, ctx { static_cast<uint8_t>(i) } {}

    public:
        uint64_t const topo;
        Smmu *   const smmu;
        Gits *   const gits;
        uint8_t  const smg;
        uint8_t  const ctx;

        auto did() const { return static_cast<uint32_t>(topo >> 32); }
        auto sid() const { return static_cast<uint32_t>(topo); }
};
