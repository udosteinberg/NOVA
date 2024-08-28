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

#include "pd.hpp"

class Mmio
{
    private:
        static inline constinit Atomic<uintptr_t> mmio_base { MMAP_GLB_MMIO };

    protected:
        uintptr_t const phys;       // Phys Base
        uintptr_t const mmio;       // MMIO Base

        explicit Mmio (uintptr_t p, size_t s) : phys { p }, mmio { mmio_base.fetch_add (s) | (p & OFFS_MASK (0)) }
        {
            // FIXME: Order 0 hardcoded. Make it based on s
            Pd::kern.Space_mem::insert (mmio, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, phys & ~OFFS_MASK (0));
        }
};
