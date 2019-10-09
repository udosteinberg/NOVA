/*
 * Generic Interrupt Controller: Distributor (GICD)
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
#include "barrier.hpp"
#include "bits.hpp"
#include "gicd.hpp"
#include "lock_guard.hpp"
#include "lowlevel.hpp"
#include "pd_kern.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"

void Gicd::init()
{
    if (EXPECT_FALSE (!phys))
        return;

    if (Cpu::bsp && !mmap_mmio())
        return;

    init_mmio();
}

bool Gicd::mmap_mmio()
{
    for (size_t size = PAGE_SIZE; size <= PAGE_SIZE << 4; size <<= 4) {

        Hptp::master.update (DEV_GLOBL_GICD, phys, bit_scan_reverse (size) - PAGE_BITS,
                             Paging::Permissions (Paging::G | Paging::W | Paging::R),
                             Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

        auto pidr = Coresight::read (Coresight::Component::PIDR2, DEV_GLOBL_GICD + size);

        if (pidr) {

            auto iidr  = read (Register32::IIDR);
            auto typer = read (Register32::TYPER);
            arch       = pidr >> 4 & BIT_RANGE (3, 0);
            ints       = 32 * ((typer & BIT_RANGE (4, 0)) + 1);
            group      = arch >= 3 || typer & BIT (10) ? GROUP1 : GROUP0;

            trace (TRACE_INTR, "GICD: %#010llx v%u r%up%u Impl:%#x Prod:%#x ESPI:%u LPIS:%u INT:%u S:%u G:%u",
                   phys, arch, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0), iidr & BIT_RANGE (11, 0), iidr >> 24,
                   arch >= 3 ? !!(typer & BIT (8)) : 0, arch >= 3 ? !!(typer & BIT (17)) : 0, ints, !!(typer & BIT (10)), group & BIT (0));

            // Reserve MMIO region
            Pd_kern::remove_user_mem (phys, size);

            return true;
        }
    }

    trace (TRACE_INTR, "GICD: MMIO unavailable. This is BAD!");

    return false;
}

void Gicd::init_mmio()
{
    auto itl = Cpu::bsp ? ints / 32 : 1;

    // Disable interrupt forwarding
    write (Register32::CTLR, 0);

    // BSP initializes all, APs SGI/PPI bank only. v3+ skips SGI/PPI bank
    for (unsigned i = arch < 3 ? 0 : 1; i < itl * 1; i++) {

        // Disable all interrupts
        write (Array32::ICENABLER, i, BIT_RANGE (31, 0));

        // Assign interrupt groups
        write (Array32::IGROUPR, i, group);
    }

    // BSP initializes all, APs SGI/PPI bank only. v3+ skips SGI/PPI bank
    for (unsigned i = arch < 3 ? 0 : 8; i < itl * 8; i++) {

        // Assign interrupt priorities
        write (Array32::IPRIORITYR, i, 0);
    }

    // Wait for completion on CTLR and ICENABLER
    wait_rwp();

    // Enable all SGIs
    if (arch < 3) {
        write (Array32::ISENABLER, 0, BIT_RANGE (15, 0));
        trace (TRACE_INTR, "GICD: Available SGIs: %#x", read (Array32::ISENABLER, 0));
    }

    // Enable interrupt forwarding
    write (Register32::CTLR, arch < 3 ? BIT (0) : BIT (4) | BIT (1));
}

void Gicd::conf (unsigned i, bool lvl, unsigned cpu)
{
    assert (i >= BASE_SPI || arch < 3);

    if (i >= ints)
        return;

    Lock_guard <Spinlock> guard (lock);

    // Configure trigger mode
    auto v = read (Array32::ICFGR, i / 16);
    v &= ~(BIT_RANGE (1, 0) << 2 * (i % 16));
    v |= static_cast<uint32>(!lvl) << (2 * (i % 16) + 1);
    write (Array32::ICFGR, i / 16, v);

    // SGI/PPI CPU targets are read-only
    if (i < BASE_SPI)
        return;

    // Configure SPI CPU target
    if (arch < 3) {
        auto t = read (Array32::ITARGETSR, i / 4);
        t &= ~(BIT_RANGE (7, 0) << 8 * (i % 4));
        t |= BIT (cpu) << 8 * (i % 4);
        write (Array32::ITARGETSR, i / 4, t);
    } else {
        uint64 affinity = Cpu::remote_affinity (cpu);
        write (Array64::IROUTER, i, (affinity & BIT_RANGE (31, 24)) << 8 | (affinity & BIT_RANGE (23, 0)));
    }
}

void Gicd::mask (unsigned i, bool masked)
{
    assert (i >= BASE_SPI || arch < 3);

    if (i >= ints)
        return;

    if (masked) {
        write (Array32::ICENABLER, i / 32, BIT (i % 32));
        wait_rwp();
    } else
        write (Array32::ISENABLER, i / 32, BIT (i % 32));
}

void Gicd::send_sgi (unsigned cpu, unsigned sgi)
{
    assert (sgi < NUM_SGI && cpu < 8 && arch < 3);

    // Ensure completion (global observability) of all previous stores
    Barrier::wmb_sync();

    write (Register32::SGIR, BIT (cpu + 16) | sgi);
}

void Gicd::wait_rwp()
{
    if (arch >= 3)
        while (read (Register32::CTLR) & BIT (31))
            pause();
}
