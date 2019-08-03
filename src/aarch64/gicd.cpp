/*
 * Generic Interrupt Controller: Distributor (GICD)
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

#include "acpi.hpp"
#include "assert.hpp"
#include "gicd.hpp"
#include "interrupt.hpp"
#include "lock_guard.hpp"
#include "space_hst.hpp"

void Gicd::init()
{
    if (!Acpi::resume && Cpu::bsp && !mmap_mmio()) [[unlikely]]
        panic ("GICD MMIO unavailable!");

    if (!init_mmio()) [[unlikely]]
        panic ("GICD initialization failed");
}

bool Gicd::mmap_mmio()
{
    if (!phys) [[unlikely]]
        return false;

    for (size_t size { PAGE_SIZE (0) }; size <= PAGE_SIZE (0) << 4; size <<= 4) {

        if (Hptp::master_map (MMAP_GLB_GICD, phys, bit_scan_msb (size) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev()) != Status::SUCCESS) [[unlikely]]
            return false;

        auto const pidr { Coresight::read (Coresight::Component::PIDR2, MMAP_GLB_GICD + size) };

        if (pidr) {

            auto const iidr  { read (Reg32::IIDR) };
            auto const typer { read (Reg32::TYPER) };

            arch  = pidr >> 4 & BIT_RANGE (3, 0);
            group = arch >= 3 || typer & BIT (10) ? GROUP1 : GROUP0;

            // Determine number of SPIs
            Interrupt::num_spi = min (NUM_SPI, (BIT_RANGE (4, 0) & typer) << 5);

            if (arch < 3) [[unlikely]] {                                                            // GICv2

                Interrupt::gsi_max = static_cast<gsi_t>(Intid::from_spi (Interrupt::num_spi) - 1);  // Max SPI
                Interrupt::iid_msk = BIT_RANGE (9, 0);                                              // INTID width is 10-bit

            } else {                                                                                // GICv3+

                auto const l { BIT_RANGE (4, 0) & typer >> 11 };
                auto const i { BIT_RANGE (4, 0) & typer >> 19 };
                auto const e { BIT_RANGE (4, 0) & typer >> 27 };

                // Determine number of ESPIs
                if (typer & BIT (8)) [[likely]]
                    Interrupt::num_espi = min (NUM_ESPI, (e + 1) << 5);

                // Determine number of LPIs
                if (typer & BIT (17)) [[likely]]
                    Interrupt::num_lpi = l ? BIT (l + 1) : Intid::to_lpi (BIT (i + 1));

                Interrupt::gsi_max = static_cast<gsi_t>(Intid::from_lpi (Interrupt::num_lpi) - 1);  // Max LPI
                Interrupt::iid_msk = BIT_RANGE (i, 0);                                              // INTID width is flexible
            }

            trace (TRACE_INTR, "GICD: %#010lx %03x:%03x r%up%u v%u ESPI:%u SPI:%u LPI:%u IID:%#x S:%u G:%u",
                   phys, iidr & BIT_RANGE (11, 0), iidr >> 24, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0),
                   arch, Interrupt::num_espi, Interrupt::num_spi, Interrupt::num_lpi, Interrupt::iid_msk, !!(typer & BIT (10)), group & BIT (0));

            // Reserve MMIO region
            Space_hst::access_ctrl (phys, size, Paging::NONE);

            return true;
        }
    }

    return false;
}

bool Gicd::init_mmio()
{
    // Disable during configuration
    if (!set_ctlr (0)) [[unlikely]]
        return false;

    if (arch < 3) [[unlikely]] {

        // GICv2 is limited to 8 CPUs
        assert (Cpu::id < 8);

        // Determine interface for this CPU
        ifid[Cpu::id] = static_cast<uint8_t>(bit_scan_lsb (read (Arr32::ITARGETSR, 0)));

        // Init SGIs and PPIs
        init_intids (Arr32::ICENABLER, Arr32::IGROUPR, Arr32::IPRIORITYR, NUM_SGI + NUM_PPI, BASE_SGI);

        // Enable all SGIs
        write (Arr32::ISENABLER, 0, BIT_RANGE (15, 0));

        // Ensure required SGIs are available
        constexpr auto sgi { BIT (Interrupt::RRQ) | BIT (Interrupt::RKE) };
        if ((read (Arr32::ISENABLER, 0) & sgi) != sgi) [[unlikely]]
            return false;
    }

    // Init SPIs and ESPIs
    if (Cpu::bsp) [[unlikely]] {
        init_intids (Arr32::ICENABLER,   Arr32::IGROUPR,   Arr32::IPRIORITYR,   Interrupt::num_spi,  BASE_SPI);
        init_intids (Arr32::ICENABLER_E, Arr32::IGROUPR_E, Arr32::IPRIORITYR_E, Interrupt::num_espi, BASE_ESPI - 4096);
    }

    // Enable interrupt forwarding
    return set_ctlr (arch < 3 ? BIT (0) : BIT (4) | BIT (1));
}

/*
 * Initialize INTID range
 *
 * @param n Number of INTIDs
 * @param o Offset in Register Block
 */
void Gicd::init_intids (Arr32 clr, Arr32 grp, Arr32 pri, unsigned n, unsigned o)
{
    // Assign interrupt groups and disable
    for (unsigned i { o }; i < o + n; i += 32) {
        write (grp, i / 32, group);
        write (clr, i / 32, BIT_RANGE (31, 0));
    }

    // Assign interrupt priorities
    for (unsigned i { o }; i < o + n; i += 4)
        write (pri, i / 4, 0);
}

bool Gicd::act_get (unsigned iid)
{
    // INTID can be SGI/PPI for GICv2 and must be a valid SPI otherwise
    assert ((arch < 3 || iid >= BASE_SPI) && iid < Intid::from_spi (Interrupt::num_spi));

    // Determine active state
    return read (Arr32::ISACTIVER, iid / 32) & BIT (iid % 32);
}

void Gicd::act_set (unsigned iid, bool act)
{
    // INTID can be SGI/PPI for GICv2 and must be a valid SPI otherwise
    assert ((arch < 3 || iid >= BASE_SPI) && iid < Intid::from_spi (Interrupt::num_spi));

    // Select SET or CLR register
    auto const reg { act ? Arr32::ISACTIVER : Arr32::ICACTIVER };
    auto const num { iid / 32 };
    auto const msk { BIT (iid % 32) };

    // Configure active state
    write (reg, num, msk);

    Barrier::fsb (Barrier::Domain::NSH);
}

void Gicd::conf_std (unsigned iid, bool lvl, bool msk, cpu_t cpu)
{
    // INTID can be SGI/PPI for GICv2 and must be a valid SPI otherwise
    assert ((arch < 3 || iid >= BASE_SPI) && iid < Intid::from_spi (Interrupt::num_spi));

    // CPU must be a valid
    assert (cpu < Cpu::count);

    Lock_guard <Spinlock> guard { lock };

    // Disable during reconfiguration
    write (Arr32::ICENABLER, iid / 32, BIT (iid % 32));

    // Wait for ICENABLER update to propagate throughout the affinity hierarchy
    wait_rwp();

    // Configure trigger mode
    auto const b { BIT (iid % 16 * 2 + 1) };
    auto const v { read (Arr32::ICFGR, iid / 16) };
    write (Arr32::ICFGR, iid / 16, lvl ? v & ~b : v | b);

    // Configure target CPU (read-only for SGI/PPI)
    if (iid >= BASE_SPI) [[likely]] {
        if (arch < 3) {
            auto t { read (Arr32::ITARGETSR, iid / 4) };
            t &= ~(BIT_RANGE (7, 0) << iid % 4 * 8);
            t |= BIT (ifid[cpu]) << iid % 4 * 8;
            write (Arr32::ITARGETSR, iid / 4, t);
        } else
            write (Arr64::IROUTER, iid, Cpu::affinity_bits (Cpu::remote_mpidr (cpu)));
    }

    // Enable as needed
    if (!msk) [[likely]]
        write (Arr32::ISENABLER, iid / 32, BIT (iid % 32));
}

void Gicd::conf_ext (unsigned iid, bool lvl, bool msk, cpu_t cpu)
{
    // INTID must be a valid ESPI
    assert (arch >= 3 && iid >= BASE_ESPI && iid < Intid::from_espi (Interrupt::num_espi));

    // CPU must be a valid
    assert (cpu < Cpu::count);

    // Adjust INTID for ESPI
    iid -= 4096;

    Lock_guard <Spinlock> guard { lock };

    // Disable during reconfiguration
    write (Arr32::ICENABLER_E, iid / 32, BIT (iid % 32));

    // Wait for ICENABLER_E update to propagate throughout the affinity hierarchy does not seem required

    // Configure trigger mode
    auto const b { BIT (iid % 16 * 2 + 1) };
    auto const v { read (Arr32::ICFGR_E, iid / 16) };
    write (Arr32::ICFGR_E, iid / 16, lvl ? v & ~b : v | b);

    // Configure target CPU
    write (Arr64::IROUTER_E, iid, Cpu::affinity_bits (Cpu::remote_mpidr (cpu)));

    // Enable as needed
    if (!msk) [[likely]]
        write (Arr32::ISENABLER_E, iid / 32, BIT (iid % 32));
}

void Gicd::send_cpu (unsigned sgi, cpu_t cpu)
{
    assert (sgi < NUM_SGI && cpu < 8 && arch < 3);

    send_sgi (BIT (16 + ifid[cpu]) | sgi);
}

void Gicd::send_exc (unsigned sgi)
{
    assert (sgi < NUM_SGI && arch < 3);

    send_sgi (BIT (24) | sgi);
}
