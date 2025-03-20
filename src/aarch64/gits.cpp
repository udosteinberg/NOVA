/*
 * Generic Interrupt Controller: Interrupt Translation Service (GITS)
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

#include "gits.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Gits::cache { sizeof (Gits), alignof (Gits) };

// GITS registers occupy two consecutive 64KB pages starting from an at least 64KB-aligned boundary
Gits::Gits (uint64_t p) : List { list }, Mmio { p, 0x20000 }, typer { read (Reg64::TYPER) }, cmdq {}
{
    auto const arch { Coresight::read (Coresight::Component::PIDR2, mmio + 0x10000) >> 4 & BIT_RANGE (3, 0) };
    auto const ctlr { read (Reg32::CTLR) };
    auto const iidr { read (Reg32::IIDR) };

    // Disable to determine coherency support
    if (set_ctlr (false)) [[likely]] {

        // Try programming desired cacheability/shareability attributes
        write (Reg64::CBASER, attr_isic);

        // Treat GITS as non-coherent if cacheability/shareability bits are not programmable
        if ((read (Reg64::CBASER) ^ attr_isic) & impl_bits) [[unlikely]]
            coherent = false;

        for (unsigned i { 0 }; i < 8; i++) {

            // Try programming desired cacheability/shareability attributes
            write (Arr64::BASER, i, attr_isic);

            // Unimplemented registers are RES0
            auto const base { read (Arr64::BASER, i) };

            // Treat GITS as non-coherent if cacheability/shareability bits are not programmable
            if (base && (base ^ attr_isic) & impl_bits) [[unlikely]]
                coherent = false;
        }
    }

    trace (TRACE_INTR, "GITS: %#010lx %03x:%03x r%up%u v%u DORD:%u EORD:%u PTA:%u VLPI:%u PLPI:%u C:%u (#%u)",
           phys, BIT_RANGE (11, 0) & iidr, iidr >> 24, BIT_RANGE (3, 0) & iidr >> 16, BIT_RANGE (3, 0) & iidr >> 12, arch,
           hwo_dev(), hwo_evt(), feat_pta(), feat_vlpi(), feat_plpi(), coherent, BIT_RANGE (3, 0) & ctlr >> 4);
}

bool Gits::init()
{
    // Disable GITS to facilitate configuration
    if (!set_ctlr (false)) [[unlikely]]
        return false;

    // Ensure command queue allocation succeeded
    if (!cmdq.ptr) [[unlikely]]
        return false;

    // Ensure command queue is 64K-aligned
    if (cmdq.get_ptr() & BIT_RANGE (15, 0)) [[unlikely]]
        return false;

    // Configure command queue (requires CTLR.Enabled == 0, CTLR.Quiescent == 1)
    write (Reg64::CBASER, (coherent ? attr_isic : attr_nsnc) | BIT64 (63) | cmdq.get_ptr() | (BIT (cmdq.pao) - 1));

    // Queue index may be non-zero upon resume
    auto const cmdq_idx { cmdq.get_idx() };

    write (Reg64::CWRITER, cmdq_idx << 5);
    write (Reg64::CREADR,  cmdq_idx << 5);

    // Configure ITS translation table descriptors (requires CTLR.Enabled == 0, CTLR.Quiescent == 1)
    for (unsigned i { 0 }; i < 8; i++)
        write (Arr64::BASER, i, baser[i]);

    // Enable GITS (requires CBASER.Valid == 1 and BASER<n>.Valid == 1)
    if (!set_ctlr (true)) [[unlikely]]
        return false;

    return true;
}
