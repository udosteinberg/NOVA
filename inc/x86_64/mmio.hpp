/*
 * Memory-Mapped I/O
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "space_hst.hpp"

class Mmio
{
    private:
        static inline constinit Atomic<uintptr_t> mmio_base { MMAP_GLB_MMIO };

    protected:
        uintptr_t const phys;       // Phys Base
        uintptr_t const mmio;       // MMIO Base

        explicit Mmio (uintptr_t p, size_t s) : phys { p }, mmio { mmio_base.fetch_add (s) | (p & OFFS_MASK (0)) }
        {
            // Reserve MMIO region
            Space_hst::access_ctrl (phys & ~OFFS_MASK (0), s, Paging::NONE);

            // FIXME: Order 0 hardcoded. Make it based on s
            Hptp::master_map (mmio, phys & ~OFFS_MASK (0), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());
        }
};
