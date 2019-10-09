/*
 * Generic Interrupt Controller: Redistributor (GICR)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "assert.hpp"
#include "bits.hpp"
#include "gicd.hpp"
#include "gicr.hpp"
#include "hpt.hpp"
#include "lowlevel.hpp"
#include "pd_kern.hpp"
#include "stdio.hpp"

void Gicr::init()
{
    if (Gicd::arch < 3 || !phys || !size)
        return;

    if (!mmap_mmio())
        return;

    init_mmio();
}

bool Gicr::mmap_mmio()
{
    // Reserve MMIO region
    if (Cpu::bsp)
        Pd_kern::remove_user_mem (phys, size);

    bool last = false;
    size_t sz = Gicd::arch < 4 ? 0x20000 : 0x40000;

    for (size_t offs = 0; offs < size && !last; offs += sz) {

        Hptp::current().update (DEV_LOCAL_GICR, phys + offs, bit_scan_reverse (sz) - PAGE_BITS,
                                Paging::Permissions (Paging::G | Paging::W | Paging::R),
                                Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

        auto pidr = Coresight::read (Coresight::Component::PIDR2, DEV_LOCAL_GICR + 0x10000);

        if (pidr) {

            auto iidr  = read (Register32::IIDR);
            auto typer = read (Register64::TYPER);
            auto arch  = pidr >> 4 & BIT_RANGE (3, 0);

            if (typer >> 32 == Cpu::affinity) {
                trace (TRACE_INTR, "GICR: %#010llx v%u r%up%u Impl:%#x Prod:%#x PPI:%llu",
                       phys + offs, arch, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0), iidr & BIT_RANGE (11, 0), iidr >> 24,
                       typer >> 27 & BIT_RANGE (4, 0));

                return true;
            }

            // Last item and nothing found
            if (typer & BIT (4))
                last = true;
        }

        Hptp::current().update (DEV_LOCAL_GICR, phys + offs, bit_scan_reverse (sz) - PAGE_BITS,
                                Paging::Permissions (0),
                                Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

        // XXX: Should flush VA
        Hptp::flush_cpu();
    }

    trace (TRACE_INTR, "GICR: MMIO unavailable. This is BAD!");

    return false;
}

void Gicr::init_mmio()
{
    // Disable LPIs
    write (Register32::CTLR, 0);

    // Disable all interrupts
    write (Register32::ICENABLER0, BIT_RANGE (31, 0));

    // Assign interrupt groups
    write (Register32::IGROUPR0, Gicd::group);

    // Assign interrupt priorities
    for (unsigned i = 0; i < 8; i++)
        write (Array32::IPRIORITYR, i, 0);

    // Wait for completion on CTLR and ICENABLER0
    wait_rwp();

    // Enable all SGIs
    write (Register32::ISENABLER0, BIT_RANGE (15, 0));
    trace (TRACE_INTR, "GICR: Available SGIs: %#x", read (Register32::ISENABLER0));

    // Mark CPU as awake
    write (Register32::WAKER, 0);
    while (read (Register32::WAKER) & BIT (2))
        pause();
}

void Gicr::conf (unsigned i, bool lvl)
{
    assert (Gicd::arch >= 3 && i < BASE_SPI);

    // Configure trigger mode
    auto v = read (Array32::ICFGR, i / 16);
    v &= ~(BIT_RANGE (1, 0) << 2 * (i % 16));
    v |= static_cast<uint32>(!lvl) << (2 * (i % 16) + 1);
    write (Array32::ICFGR, i / 16, v);
}

void Gicr::mask (unsigned i, bool masked)
{
    assert (Gicd::arch >= 3 && i < BASE_SPI);

    if (masked) {
        write (Register32::ICENABLER0, BIT (i % 32));
        wait_rwp();
    } else
        write (Register32::ISENABLER0, BIT (i % 32));
}

void Gicr::wait_rwp()
{
    assert (Gicd::arch >= 3);

    while (read (Register32::CTLR) & BIT (3))
        pause();
}
