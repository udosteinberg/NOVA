/*
 * Generic Interrupt Controller: Redistributor (GICR)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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
#include "stdio.hpp"

void Gicr::wait_rwp()
{
    assert (Gicd::arch >= 3);

    while (read (Register32::CTLR) & BIT (3))
        pause();
}

void Gicr::init()
{
    if (Gicd::arch < 3 || !GICR_SIZE)
        return;

    for (unsigned offs = 0; offs < GICR_SIZE; offs += 0x20000) {

        Hptp::current().update (DEV_LOCAL_GICR, GICR_BASE + offs, bit_scan_reverse (0x20000) - PAGE_BITS,
                                Paging::Permissions (Paging::R | Paging::W | Paging::G),
                                Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

        auto pidr = Coresight::read (Coresight::Component::PIDR2, DEV_LOCAL_GICR + 0x10000);

        if (pidr) {

            auto iidr  = read (Register32::IIDR);
            auto typer = read (Register64::TYPER);
            auto arch  = pidr >> 4 & 0xf;

            if (typer >> 32 == Cpu::affinity) {
                trace (TRACE_INTR, "GICR: %#010x Impl:%#x Prod:%#x r%up%u (v%u) PPI:%llu",
                       GICR_BASE + offs,
                       iidr & 0xfff, iidr >> 24, iidr >> 16 & 0xf, iidr >> 12 & 0xf,
                       arch, typer >> 27 & 0x1f);

                break;
            }

            // Last item and nothing found
            if (typer & BIT (4))
                return;
        }

        Hptp::current().update (DEV_LOCAL_GICR, GICR_BASE + offs, bit_scan_reverse (0x20000) - PAGE_BITS,
                                Paging::Permissions (0),
                                Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

        // XXX: Should flush VA
        Hptp::flush_cpu();
    }

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

void Gicr::conf (unsigned i, bool edge)
{
    assert (i < SPI_BASE && Gicd::arch >= 3);

    // Configure trigger mode
    auto v = read (Array32::ICFGR, i / 16);
    v &= ~(0x3U << 2 * (i % 16));
    v |= (static_cast<uint32>(edge) << 1) << 2 * (i % 16);
    write (Array32::ICFGR, i / 16, v);
}

void Gicr::mask (unsigned i, bool masked)
{
    assert (i < SPI_BASE && Gicd::arch >= 3);

    if (masked) {
        write (Register32::ICENABLER0, BIT (i % 32));
        wait_rwp();
    } else
        write (Register32::ISENABLER0, BIT (i % 32));
}
