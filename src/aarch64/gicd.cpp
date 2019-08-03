/*
 * Generic Interrupt Controller: Distributor (GICD)
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
            Interrupt::num_spi = min (NUM_SPI, (typer & BIT_RANGE (4, 0)) << 5);

            if (arch >= 3) [[likely]] {

                auto const e { typer >> 27 & BIT_RANGE (4, 0) };
                auto const l { typer >> 11 & BIT_RANGE (4, 0) };

                // Determine order of INTID bits
                ord_iid = (typer >> 19 & BIT_RANGE (4, 0)) + 1;

                // Determine number of ESPIs
                if (typer & BIT (8)) [[likely]]
                    Interrupt::num_espi = min (NUM_ESPI, (e + 1) << 5);

                // Determine number of LPIs
                if (typer & BIT (17)) [[likely]]
                    Interrupt::num_lpi = min (NUM_LPI, l ? BIT (l + 1) : Intid::to_lpi (BIT (ord_iid)));
            }

            trace (TRACE_INTR, "GICD: %#010lx %03x:%03x r%up%u v%u ESPI:%u SPI:%u LPI:%u IID:%u S:%u G:%u",
                   phys, iidr & BIT_RANGE (11, 0), iidr >> 24, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0),
                   arch, Interrupt::num_espi, Interrupt::num_spi, Interrupt::num_lpi, ord_iid, !!(typer & BIT (10)), group & BIT (0));

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
        init_intid (Arr32::ICENABLER, Arr32::IGROUPR, Arr32::IPRIORITYR, BASE_SGI, NUM_SGI + NUM_PPI);

        // Enable all SGIs
        write (Arr32::ISENABLER, 0, BIT_RANGE (15, 0));

        // Ensure required SGIs are available
        constexpr auto sgi { BIT (Interrupt::RRQ) | BIT (Interrupt::RKE) };
        if ((read (Arr32::ISENABLER, 0) & sgi) != sgi) [[unlikely]]
            return false;
    }

    // Init SPIs and ESPIs
    if (Cpu::bsp) [[unlikely]] {
        init_intid (Arr32::ICENABLER,   Arr32::IGROUPR,   Arr32::IPRIORITYR,   BASE_SPI,         Interrupt::num_spi);
        init_intid (Arr32::ICENABLER_E, Arr32::IGROUPR_E, Arr32::IPRIORITYR_E, BASE_ESPI - 4096, Interrupt::num_espi);
    }

    // Enable interrupt forwarding
    return set_ctlr (arch < 3 ? BIT (0) : BIT (4) | BIT (1));
}

/*
 * Initialize INTID range
 */
void Gicd::init_intid (Arr32 clr, Arr32 grp, Arr32 pri, arm_intid_t iid, unsigned n)
{
    // Assign interrupt groups and disable
    for (auto i { iid }; i < iid + n; i += 32) {
        write (grp, i / 32, group);
        write (clr, i / 32, BIT_RANGE (31, 0));
    }

    // Assign interrupt priorities
    for (auto i { iid }; i < iid + n; i += 4)
        write (pri, i / 4, 0);
}

bool Gicd::act_get (arm_intid_t iid)
{
    // INTID can be SGI/PPI for GICv2 and must be a valid SPI otherwise
    assert ((arch < 3 || iid >= BASE_SPI) && iid < Intid::from_spi (Interrupt::num_spi));

    // Determine active state
    return read (Arr32::ISACTIVER, iid / 32) & BIT (iid % 32);
}

void Gicd::act_set (arm_intid_t iid, bool act)
{
    // INTID can be SGI/PPI for GICv2 and must be a valid SPI otherwise
    assert ((arch < 3 || iid >= BASE_SPI) && iid < Intid::from_spi (Interrupt::num_spi));

    // Configure active state
    write (act ? Arr32::ISACTIVER : Arr32::ICACTIVER, iid / 32, BIT (iid % 32));

    /*
     * According to Arm DDI0601 (032025) for ISACTIVER<n>:
     * The effect of a write must be visible in finite time. Reading back
     * the written value from ISACTIVER guarantees that a deactivate from a
     * CPU interface, ordered-after the read, will observe the effects of
     * the write on the active state of the interrupt.
     */
    Barrier::fsb (Barrier::Domain::NSH);
}

Status Gicd::conf_std (arm_intid_t iid, cpu_t cpu, bool lvl, bool msk)
{
    // INTID can be SGI/PPI for GICv2 and must be a valid SPI otherwise
    assert ((arch < 3 || iid >= BASE_SPI) && iid < Intid::from_spi (Interrupt::num_spi));

    // CPU must be valid
    assert (cpu < Cpu::count);

    Lock_guard <Spinlock> guard { lock };

    // Disable during reconfiguration
    write (Arr32::ICENABLER, iid / 32, BIT (iid % 32));

    // Wait for ICENABLER update to propagate throughout the affinity hierarchy
    if (!wait_rwp()) [[unlikely]]
        return Status::ABORTED;

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

    return Status::SUCCESS;
}

Status Gicd::conf_ext (arm_intid_t iid, cpu_t cpu, bool lvl, bool msk)
{
    // INTID must be a valid ESPI
    assert (arch >= 3 && iid >= BASE_ESPI && iid < Intid::from_espi (Interrupt::num_espi));

    // CPU must be valid
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

    return Status::SUCCESS;
}

void Gicd::send_cpu (unsigned sgi, cpu_t cpu)
{
    assert (arch < 3 && sgi < NUM_SGI && cpu < 8);

    send_sgi (BIT (16 + ifid[cpu]) | sgi);
}

void Gicd::send_exc (unsigned sgi)
{
    assert (arch < 3 && sgi < NUM_SGI);

    send_sgi (BIT (24) | sgi);
}
