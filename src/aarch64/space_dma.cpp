/*
 * DMA Memory Space
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

#include "space_dma.hpp"

Space_dma *Space_dma::create (Status &s, Pd *pd)
{
    // Acquire reference
    Refptr<Pd> ref_pd { pd };

    // Failed to acquire reference
    if (!ref_pd) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Allocate SDID
        if (auto const r { Sdid::allocator.alloc() }) [[likely]] {

            auto const sdid { r.val() };

            // Create new DMA object
            auto const obj { new (ref_pd->dma_cache) Space_dma { ref_pd, sdid } };

            // If creation succeeded, then reference must have been consumed
            if (obj) [[likely]] {

                assert (!ref_pd);

                // DPT root will be initialized by get_root() later
                return obj;
            }

            Sdid::allocator.free (sdid);
        }

        // Failed to create DMA object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Space_dma::destroy()
{
    auto &cache { get_pd()->dma_cache };

    this->~Space_dma();

    operator delete (this, cache);
}
