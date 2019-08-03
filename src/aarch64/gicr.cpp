/*
 * Generic Interrupt Controller: Redistributor (GICR)
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
#include "gicr.hpp"
#include "interrupt.hpp"
#include "space_hst.hpp"

void Gicr::init()
{
    if (Gicd::arch < 3)
        return;

    if (!Acpi::resume) {

        // If GICR has not been enumerated by ACPI, then use board values
        if (Cpu::bsp && !Cpu::gicr)
            if (!enumerate (Board::gic[1].mmio, Board::gic[1].size)) [[unlikely]]
                panic ("%s failure", __PRETTY_FUNCTION__);

        if (!mmap_mmio()) [[unlikely]]
            panic ("GICR MMIO unavailable!");
    }

    if (!init_mmio()) [[unlikely]]
        panic ("GICR initialization failed");
}

bool Gicr::enumerate (uint64_t phys, uint32_t size)
{
    // CPU count must be known
    assert (Cpu::count);

    auto const map { reinterpret_cast<uintptr_t>(Hptp::map_tmp (phys, size, Paging::R, Memattr::dev(), 1)) };
    if (!map) [[unlikely]]
        return false;

    for (auto ptr { map }; ptr < map + size; ) {

        auto const type { *std::start_lifetime_as<uint64_t volatile> (ptr + std::to_underlying (Reg64::TYPER)) };

        // Assign GICR address to its CPU
        for (cpu_t cpu { 0 }; cpu < Cpu::count; cpu++)
            if (type >> 32 == Cpu::affinity_pack (Cpu::remote_mpidr (cpu)))
                *Kmem::loc_to_glb (cpu, &Cpu::gicr) = ptr - map + phys;

        // Stop at the last redistributor
        if (type & BIT (4))
            return true;

        // vLPIs double the region size
        ptr += type & BIT (1) ? 2 * mmio_size : mmio_size;
    }

    return false;
}

bool Gicr::mmap_mmio()
{
    assert (Gicd::ord_iid);

    if (!Cpu::gicr) [[unlikely]]
        return false;

    // Map MMIO region
    if (Hptp::current().update (MMAP_CPU_GICR, Cpu::gicr, bit_scan_msb (mmio_size) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev()) != Status::SUCCESS) [[unlikely]]
        return false;

    auto const arch { Coresight::read (Coresight::Component::PIDR2, MMAP_CPU_GICR + 0x10000) >> 4 & BIT_RANGE (3, 0) };
    auto const iidr { read (Reg32::IIDR) };
    auto const type { read (Reg64::TYPER) };

    // Determine GICR PE number
    Cpu::gicr_pe = static_cast<uint16_t>(type >> 8);

    // Disable to determine coherence support
    if (!set_ctlr (0)) [[unlikely]]
        return false;

    // Try programming desired cacheability/shareability attributes (requires CTLR.EnableLPIs == 0)
    write (Reg64::PROPBASER, attr_isic);
    write (Reg64::PENDBASER, attr_isic);

    // Treat GICR as noncoherent if cacheability/shareability bits are not programmable
    if (((read (Reg64::PROPBASER) ^ attr_isic) | (read (Reg64::PENDBASER) ^ attr_isic)) & impl_bits) [[unlikely]]
        noncoherent = true;

    // Determine number of EPPIs
    if (Cpu::bsp) [[unlikely]]
        Interrupt::num_eppi = min (Intid::NUM_EPPI, static_cast<unsigned>(type >> 22 & BIT_RANGE (9, 5)));

    // Set up LPIs if supported
    if (type & BIT (0) && Gicd::ord_iid > bit_scan_msb (Intid::BASE_LPI)) [[likely]] {

        // Global LPI configuration table
        if (Cpu::bsp) [[unlikely]]
            cfg_table = new (noncoherent, Gicd::ord_iid) Cfg_table;

        // CPU-local LPI pending table
        pnd_table = new (noncoherent, Gicd::ord_iid) Pnd_table;

        // Ensure alignment constraints
        assert ((Kmem::ptr_to_phys (cfg_table) & ALIGNMENT_OFFS (12)) == 0);
        assert ((Kmem::ptr_to_phys (pnd_table) & ALIGNMENT_OFFS (16)) == 0);
    }

    trace (TRACE_INTR, "GICR: %#010lx %03x:%03x r%up%u v%u EPPI:%u MPAM:%u DLPI:%u VLPI:%u PLPI:%u NC:%u (#%u)",
           Cpu::gicr, iidr & BIT_RANGE (11, 0), iidr >> 24, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0), arch,
           Interrupt::num_eppi, !!(type & BIT (6)), !!(type & BIT (3)), !!(type & BIT (1)), !!(type & BIT (0)), noncoherent, Cpu::gicr_pe);

    // Reserve MMIO region
    Space_hst::access_ctrl (Cpu::gicr, type & BIT (1) ? 2 * mmio_size : mmio_size, Paging::NONE);

    return true;
}

bool Gicr::init_mmio()
{
    // Disable during configuration
    if (!set_ctlr (0)) [[unlikely]]
        return false;

    // Init SGIs and PPIs
    init_intid (Intid::BASE_SGI, Intid::NUM_SGI + Intid::NUM_PPI);

    // Enable all SGIs
    write (Arr32::ISENABLER, 0, BIT_RANGE (15, 0));

    // Ensure required SGIs are available
    constexpr auto sgi { BIT (Interrupt::RRQ) | BIT (Interrupt::RKE) };
    if ((read (Arr32::ISENABLER, 0) & sgi) != sgi) [[unlikely]]
        return false;

    // Init EPPIs
    init_intid (Intid::BASE_EPPI - 1024, Interrupt::num_eppi);

    auto const type { read (Reg64::TYPER) };

    // Init LPIs if supported (requires CTLR.EnableLPIs == 0)
    if (type & BIT (0) && Gicd::ord_iid > bit_scan_msb (Intid::BASE_LPI)) [[likely]] {

        // Ensure data structure allocations succeeded
        if (!cfg_table || !pnd_table) [[unlikely]]
            return false;

        // Configure configuration and pending table
        write (Reg64::PROPBASER, (noncoherent ? attr_nsnc : attr_isic) | Kmem::ptr_to_phys (cfg_table) | (Gicd::ord_iid - 1));
        write (Reg64::PENDBASER, (noncoherent ? attr_nsnc : attr_isic) | Kmem::ptr_to_phys (pnd_table));

        // Enable LPIs
        if (!set_ctlr (BIT (0))) [[unlikely]]
            return false;
    }

    // Wake CPU interface
    return set_sleep (false);
}

/*
 * Initialize INTID range
 */
void Gicr::init_intid (uint32_t iid, unsigned n)
{
    // Assign interrupt groups and disable
    for (auto i { iid }; i < iid + n; i += 32) {
        write (Arr32::IGROUPR,   i / 32, Gicd::group);
        write (Arr32::ICENABLER, i / 32, BIT_RANGE (31, 0));
    }

    // Assign interrupt priorities
    for (auto i { iid }; i < iid + n; i += 4)
        write (Arr32::IPRIORITYR, i / 4, 0);
}

bool Gicr::act_get (Intid iid)
{
    // INTID must be a valid PPI or EPPI
    assert ((iid >= Intid::BASE_PPI && iid < Intid::from_ppi (Intid::NUM_PPI)) || (Gicd::arch >= 3 && iid >= Intid::BASE_EPPI && iid < Intid::from_eppi (Interrupt::num_eppi)));

    // For GIC versions < 3, PPIs reside in GICD
    if (Gicd::arch < 3) [[unlikely]]
        return Gicd::act_get (iid);

    // Adjust INTID for EPPI register block
    uint32_t reg { iid };
    if (reg >= Intid::BASE_EPPI) [[unlikely]]
        reg -= 1024;

    // Determine active state
    return read (Arr32::ISACTIVER, reg / 32) & BIT (reg % 32);
}

void Gicr::act_set (Intid iid, bool act)
{
    // INTID must be a valid PPI or EPPI
    assert ((iid >= Intid::BASE_PPI && iid < Intid::from_ppi (Intid::NUM_PPI)) || (Gicd::arch >= 3 && iid >= Intid::BASE_EPPI && iid < Intid::from_eppi (Interrupt::num_eppi)));

    // For GIC versions < 3, PPIs reside in GICD
    if (Gicd::arch < 3) [[unlikely]]
        return Gicd::act_set (iid, act);

    // Adjust INTID for EPPI register block
    uint32_t reg { iid };
    if (reg >= Intid::BASE_EPPI) [[unlikely]]
        reg -= 1024;

    // Configure active state
    write (act ? Arr32::ISACTIVER : Arr32::ICACTIVER, reg / 32, BIT (reg % 32));

    // Device-nGnRnE + DSB ensures completion at GICR register level
    Barrier::fsb (Barrier::Domain::NSH);
}

Status Gicr::conf_ppi (Intid iid, bool lvl, bool msk)
{
    // INTID must be a valid PPI or EPPI
    assert ((iid >= Intid::BASE_PPI && iid < Intid::from_ppi (Intid::NUM_PPI)) || (Gicd::arch >= 3 && iid >= Intid::BASE_EPPI && iid < Intid::from_eppi (Interrupt::num_eppi)));

    // For GIC versions < 3, PPIs reside in GICD
    if (Gicd::arch < 3) [[unlikely]]
        return Gicd::conf_std (iid, 0, lvl, msk);

    // Adjust INTID for EPPI register block
    uint32_t reg { iid };
    if (reg >= Intid::BASE_EPPI) [[unlikely]]
        reg -= 1024;

    // Disable during reconfiguration
    write (Arr32::ICENABLER, reg / 32, BIT (reg % 32));

    // Wait for ICENABLER update to propagate throughout the affinity hierarchy
    if (!wait_rwp()) [[unlikely]]
        return Status::ABORTED;

    // Configure trigger mode
    auto const b { BIT (reg % 16 * 2 + 1) };
    auto const v { read (Arr32::ICFGR, reg / 16) };
    write (Arr32::ICFGR, reg / 16, lvl ? v & ~b : v | b);

    // Enable as needed
    if (!msk) [[likely]]
        write (Arr32::ISENABLER, reg / 32, BIT (reg % 32));

    return Status::SUCCESS;
}

void Gicr::conf_lpi (Intid iid, bool msk)
{
    // INTID must be a valid LPI
    assert (Gicd::arch >= 3 && iid >= Intid::BASE_LPI && iid < Intid::from_lpi (Interrupt::num_lpi));

    // Global LPI configuration table must exist
    assert (cfg_table);

    // Enable as needed
    cfg_table[Intid::to_lpi (iid)].set (noncoherent, msk);
}
