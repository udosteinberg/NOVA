/*
 * Generic Interrupt Controller: Distributor (GICD)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "lock_guard.hpp"
#include "lowlevel.hpp"
#include "ptab_hpt.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"

void Gicd::init()
{
    if (!Acpi::resume && Cpu::bsp && !mmap_mmio())
        Console::panic ("GICD: MMIO unavailable!");

    init_mmio();
}

bool Gicd::mmap_mmio()
{
    if (!phys)
        return false;

    for (size_t size { PAGE_SIZE }; size <= PAGE_SIZE << 4; size <<= 4) {

        Hptp::master_map (MMAP_GLB_GICD, phys, bit_scan_reverse (size) - PAGE_BITS,
                          Paging::Permissions (Paging::G | Paging::W | Paging::R),
                          Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

        auto const pidr { Coresight::read (Coresight::Component::PIDR2, MMAP_GLB_GICD + size) };

        if (pidr) {

            auto const iidr  { read (Register32::IIDR) };
            auto const typer { read (Register32::TYPER) };

            arch  = pidr >> 4 & BIT_RANGE (3, 0);
            intid = min (32 * ((typer & BIT_RANGE (4, 0)) + 1), Intid::BASE_RSV);
            group = arch >= 3 || typer & BIT (10) ? GROUP1 : GROUP0;

            trace (TRACE_INTR, "GICD: %#010llx v%u r%up%u Impl:%#x Prod:%#x ESPI:%u LPIS:%u INT:%u S:%u G:%u",
                   phys, arch, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0), iidr & BIT_RANGE (11, 0), iidr >> 24,
                   arch >= 3 ? !!(typer & BIT (8)) : 0, arch >= 3 ? !!(typer & BIT (17)) : 0, intid, !!(typer & BIT (10)), group & BIT (0));

            // Reserve MMIO region
            Space_hst::user_access (phys, size, false);

            return true;
        }
    }

    return false;
}

void Gicd::init_mmio()
{
    // Disable interrupt forwarding
    write (Register32::CTLR, 0);

    // BSP initializes all, APs SGI/PPI bank only. v3+ skips SGI/PPI bank
    auto const s { arch < 3 ? BASE_SGI : BASE_SPI };
    auto const e { Cpu::bsp ? intid : BASE_SPI };

    // Assign interrupt groups and disable
    for (unsigned i { s }; i < e; i += 32) {
        write (Array32::ICENABLER, i / 32, BIT_RANGE (31, 0));
        write (Array32::IGROUPR, i / 32, group);
    }

    // Assign interrupt priorities
    for (unsigned i { s }; i < e; i += 4)
        write (Array32::IPRIORITYR, i / 4, 0);

    // Wait for completion on CTLR and ICENABLER
    wait_rwp();

    if (arch < 3) {

        // Determine interface for this CPU
        ifid[Cpu::id] = static_cast<uint8>(bit_scan_forward (read (Array32::ITARGETSR, 0)));

        // Enable all SGIs
        write (Array32::ISENABLER, 0, BIT_RANGE (15, 0));

        // Ensure our SGIs are available
        assert (read (Array32::ISENABLER, 0) & BIT_RANGE (1, 0));
    }

    // Enable interrupt forwarding
    write (Register32::CTLR, arch < 3 ? BIT (0) : BIT (4) | BIT (1));
}

bool Gicd::get_act (unsigned i)
{
    assert (i >= BASE_SPI || arch < 3);
    assert (i <  BASE_RSV && i < intid);

    return read (Array32::ISACTIVER, i / 32) & BIT (i % 32);
}

void Gicd::set_act (unsigned i, bool a)
{
    assert (i >= BASE_SPI || arch < 3);
    assert (i <  BASE_RSV && i < intid);

    write (a ? Array32::ISACTIVER : Array32::ICACTIVER, i / 32, BIT (i % 32));

    Barrier::fsb (Barrier::NSH);
}

void Gicd::conf (unsigned i, bool msk, bool lvl, unsigned cpu)
{
    assert (i >= BASE_SPI || arch < 3);
    assert (i <  BASE_RSV && i < intid);

    Lock_guard <Spinlock> guard { lock };

    // Mask during reconfiguration
    write (Array32::ICENABLER, i / 32, BIT (i % 32));
    wait_rwp();

    // Configure trigger mode
    auto const b { BIT (i % 16 * 2 + 1) };
    auto const v { read (Array32::ICFGR, i / 16) };
    write (Array32::ICFGR, i / 16, lvl ? v & ~b : v | b);

    // Configure target CPU for SPI (read-only for SGI/PPI)
    if (i >= BASE_SPI) {
        if (arch < 3) {
            auto t { read (Array32::ITARGETSR, i / 4) };
            t &= ~(BIT_RANGE (7, 0) << i % 4 * 8);
            t |= BIT (ifid[cpu]) << i % 4 * 8;
            write (Array32::ITARGETSR, i / 4, t);
        } else
            write (Array64::IROUTER, i, Cpu::affinity_bits (Cpu::remote_mpidr (cpu)));
    }

    // Finalize mask state
    if (!msk)
        write (Array32::ISENABLER, i / 32, BIT (i % 32));
}

void Gicd::send_cpu (unsigned sgi, unsigned cpu)
{
    assert (sgi < NUM_SGI && cpu < 8 && arch < 3);

    send_sgi (BIT (16 + ifid[cpu]) | sgi);
}

void Gicd::send_exc (unsigned sgi)
{
    assert (sgi < NUM_SGI && arch < 3);

    send_sgi (BIT (24) | sgi);
}

void Gicd::wait_rwp()
{
    if (arch >= 3)
        while (read (Register32::CTLR) & BIT (31))
            pause();
}
