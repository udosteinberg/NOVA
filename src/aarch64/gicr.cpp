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
#include "ptab_hpt.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"

void Gicr::init()
{
    if (Gicd::arch < 3)
        return;

    if (!Acpi::resume) {

        // If GICR has not been enumerated by ACPI, then use board values
        if (Cpu::bsp && !Cpu::gicr)
            assign (Board::gic[1].mmio);

        mmap_mmio();
    }

    init_mmio();
}

void Gicr::assign (uint64_t phys)
{
    auto const map { reinterpret_cast<uintptr_t>(Hptp::map (MMAP_GLB_MAP1, phys, Paging::R, Memattr::dev())) };

    for (auto ptr { map }; ptr < map + Hpt::page_size (Hpt::bpl); ) {

        auto const type { *reinterpret_cast<uint64_t volatile *>(ptr + std::to_underlying (Reg64::TYPER)) };

        // Assign GICR address to its CPU
        for (cpu_t cpu { 0 }; cpu < Cpu::count; cpu++)
            if (type >> 32 == Cpu::affinity_pack (Cpu::remote_mpidr (cpu)))
                *Kmem::loc_to_glob (cpu, &Cpu::gicr) = ptr - map + phys;

        // Stop at the last redistributor
        if (type & BIT (4))
            break;

        // vLPIs double the region size
        ptr += type & BIT (1) ? 2 * mmio_size : mmio_size;
    }
}

void Gicr::mmap_mmio()
{
    if (EXPECT_FALSE (!Cpu::gicr))
        Console::panic ("GICR: %s failed!", __func__);

    // Map MMIO region
    Hptp::current().update (MMAP_CPU_GICR, Cpu::gicr, bit_scan_reverse (mmio_size) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    // Reserve MMIO region
    Space_hst::user_access (Cpu::gicr, read (Reg64::TYPER) & BIT (1) ? 2 * mmio_size : mmio_size, false);
}

void Gicr::init_mmio()
{
    auto const arch { Coresight::read (Coresight::Component::PIDR2, MMAP_CPU_GICR + 0x10000) >> 4 & BIT_RANGE (3, 0) };
    auto const iidr { read (Reg32::IIDR) };
    auto const type { read (Reg64::TYPER) };

    trace (TRACE_INTR, "GICR: %#010lx v%u r%up%u Impl:%#x Prod:%#x EPPI:%u MPAM:%u",
           Cpu::gicr, arch, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0), iidr & BIT_RANGE (11, 0), iidr >> 24,
           !!(type >> 27 & BIT_RANGE (4, 0)), !!(type & BIT (6)));

    // Disable LPIs
    write (Reg32::CTLR, 0);

    // Disable all interrupts
    write (Reg32::ICENABLER0, BIT_RANGE (31, 0));

    // Assign interrupt groups
    write (Reg32::IGROUPR0, Gicd::group);

    // Assign interrupt priorities
    for (unsigned i { 0 }; i < 8; i++)
        write (Arr32::IPRIORITYR, i, 0);

    // Wait for completion on CTLR and ICENABLER0
    wait_rwp();

    // Enable all SGIs
    write (Reg32::ISENABLER0, BIT_RANGE (15, 0));

    // Ensure our SGIs are available
    assert (read (Reg32::ISENABLER0) & BIT_RANGE (1, 0));

    // Mark CPU as awake
    write (Reg32::WAKER, 0);
    while (read (Reg32::WAKER) & BIT (2))
        pause();
}

bool Gicr::get_act (unsigned i)
{
    assert (i < BASE_SPI && Gicd::arch >= 3);

    return read (Reg32::ISACTIVER0) & BIT (i);
}

void Gicr::set_act (unsigned i, bool a)
{
    assert (i < BASE_SPI && Gicd::arch >= 3);

    write (a ? Reg32::ISACTIVER0 : Reg32::ICACTIVER0, BIT (i));

    Barrier::fsb (Barrier::Domain::NSH);
}

void Gicr::conf (unsigned i, bool msk, bool lvl)
{
    assert (i < BASE_SPI && Gicd::arch >= 3);

    // Mask during reconfiguration
    write (Reg32::ICENABLER0, BIT (i));
    wait_rwp();

    // Configure trigger mode
    auto const b { BIT (i % 16 * 2 + 1) };
    auto const v { read (Arr32::ICFGR, i / 16) };
    write (Arr32::ICFGR, i / 16, lvl ? v & ~b : v | b);

    // Finalize mask state
    if (!msk)
        write (Reg32::ISENABLER0, BIT (i));
}

void Gicr::wait_rwp()
{
    while (read (Reg32::CTLR) & BIT (3))
        pause();
}
