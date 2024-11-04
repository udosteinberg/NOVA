/*
 * Memory-Mapped I/O
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

#include "ptab_hpt.hpp"
#include "space_hst.hpp"

class Mmio
{
    private:
        static inline constinit Atomic<uintptr_t> mmio_base { MMAP_GLB_MMIO };

    protected:
        uintptr_t const phys;       // Phys Base
        uintptr_t const mmio;       // MMIO Base
        size_t    const mmio_size;  // MMIO Size

        static uintptr_t alloc_mmio (uintptr_t const phys, size_t const size, Memattr const a, bool const r)
        {
            // Skip MMIO allocation if size is 0
            if (!size) [[unlikely]]
                return 0;

            // Round physical address and size to full pages
            auto p { aligned_dn (Hpt::page_size (0), phys) };
            auto s { aligned_up (Hpt::page_size (0), phys + size) - p };

            // Reserve physical memory region
            if (r) [[likely]]
                Space_hst::access_ctrl (p, s, Paging::NONE);

            // Allocate MMIO region
            auto v { mmio_base.fetch_add (s) };
            auto m { v | (phys & Hpt::offs_mask (0)) };

            // Map MMIO region
            for (unsigned o; s; s -= BITN (o), p += BITN (o), v += BITN (o))
                Hptp::master_map (v, p, (o = aligned_order (s, p, v)) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), a);

            return m;
        }

        explicit Mmio (uintptr_t p, size_t s, Memattr a, bool r = true) : phys { p }, mmio { alloc_mmio (p, s, a, r) }, mmio_size { s } {}
};
