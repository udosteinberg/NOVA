/*
 * Generic Interrupt Controller: Redistributor (GICR)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "acpi.hpp"
#include "assert.hpp"
#include "bits.hpp"
#include "gicd.hpp"
#include "gicr.hpp"
#include "lowlevel.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"

void Gicr::init()
{
    if (Gicd::arch < 3)
        return;

    if (!Acpi::resume && !mmap_mmio())
        Console::panic ("GICR: MMIO unavailable!");

    init_mmio();
}

bool Gicr::mmap_mmio()
{
    if (!phys)
        return false;

    for (uint64_t addr { phys }, size;; addr += size) {

        Hptp::current().update (MMAP_CPU_GICR, addr, bit_scan_reverse (0x20000) - PAGE_BITS,
                                Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

        Hptp::invalidate_cpu();     // XXX: Should invalidate VA

        auto const pidr { Coresight::read (Coresight::Component::PIDR2, MMAP_CPU_GICR + 0x10000) };

        if (!pidr)
            break;

        auto const iidr { read (Register32::IIDR) };
        auto const type { read (Register64::TYPER) };
        auto const arch { pidr >> 4 & BIT_RANGE (3, 0) };

        // vLPIs double the region size
        size = type & BIT (1) ? 0x40000 : 0x20000;

        if (type >> 32 == Cpu::affinity()) {

            trace (TRACE_INTR, "GICR: %#010lx v%u r%up%u Impl:%#x Prod:%#x EPPI:%u MPAM:%u",
                   addr, arch, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0), iidr & BIT_RANGE (11, 0), iidr >> 24,
                   !!(type >> 27 & BIT_RANGE (4, 0)), !!(type & BIT (6)));

            // Reserve MMIO region
            Space_hst::user_access (addr, size, false);

            return true;
        }

        // Last item
        if (type & BIT (4))
            break;
    }

    Hptp::current().update (MMAP_CPU_GICR, 0, bit_scan_reverse (0x20000) - PAGE_BITS,
                            Paging::NONE, Memattr::dev());

    Hptp::invalidate_cpu();         // XXX: Should invalidate VA

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
    for (unsigned i { 0 }; i < 8; i++)
        write (Array32::IPRIORITYR, i, 0);

    // Wait for completion on CTLR and ICENABLER0
    wait_rwp();

    // Enable all SGIs
    write (Register32::ISENABLER0, BIT_RANGE (15, 0));

    // Ensure our SGIs are available
    assert (read (Register32::ISENABLER0) & BIT_RANGE (1, 0));

    // Mark CPU as awake
    write (Register32::WAKER, 0);
    while (read (Register32::WAKER) & BIT (2))
        pause();
}

bool Gicr::get_act (unsigned i)
{
    assert (i < BASE_SPI && Gicd::arch >= 3);

    return read (Register32::ISACTIVER0) & BIT (i);
}

void Gicr::set_act (unsigned i, bool a)
{
    assert (i < BASE_SPI && Gicd::arch >= 3);

    write (a ? Register32::ISACTIVER0 : Register32::ICACTIVER0, BIT (i));

    Barrier::fsb (Barrier::Domain::NSH);
}

void Gicr::conf (unsigned i, bool msk, bool lvl)
{
    assert (i < BASE_SPI && Gicd::arch >= 3);

    // Mask during reconfiguration
    write (Register32::ICENABLER0, BIT (i));
    wait_rwp();

    // Configure trigger mode
    auto const b { BIT (i % 16 * 2 + 1) };
    auto const v { read (Array32::ICFGR, i / 16) };
    write (Array32::ICFGR, i / 16, lvl ? v & ~b : v | b);

    // Finalize mask state
    if (!msk)
        write (Register32::ISENABLER0, BIT (i));
}

void Gicr::wait_rwp()
{
    while (read (Register32::CTLR) & BIT (3))
        pause();
}
