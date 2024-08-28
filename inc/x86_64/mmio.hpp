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

#include "bits.hpp"
#include "pd.hpp"

class Mmio
{
    private:
        static inline constinit Atomic<uintptr_t> mmio_base { MMAP_GLB_MMIO };

    protected:
        uintptr_t const phys;       // Phys Base
        uintptr_t const mmio;       // MMIO Base

        static auto alloc_mmio (uintptr_t const phys, size_t const size)
        {
            // Round physical address and size to full pages
            auto p { aligned_dn (PAGE_SIZE (0), phys) };
            auto s { aligned_up (PAGE_SIZE (0), phys + size) - p };

            // Allocate MMIO region
            auto v { mmio_base.fetch_add (s) };
            auto r { v | (phys & OFFS_MASK (0)) };

            // Map MMIO region
            for (unsigned o; s; s -= BITN (o), p += BITN (o), v += BITN (o))
                Pd::kern.Space_mem::insert (v, (o = aligned_order (s, p, v)) - PAGE_BITS, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, p);

            return r;
        }

        explicit Mmio (uintptr_t p, size_t s) : phys { p }, mmio { alloc_mmio (p, s) } {}
};
