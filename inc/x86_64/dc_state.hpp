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

#include "smmu.hpp"

class Dc_state
{
    public:
        pci_t  const sbdf;
        Smmu * const smmu;

    protected:
        explicit Dc_state (uint64_t t, uint64_t d, uint64_t) : sbdf { static_cast<pci_t>(t) }, smmu { Smmu::lookup (d & ~OFFS_MASK (0)) } {}
};
