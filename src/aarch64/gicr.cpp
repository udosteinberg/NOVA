/*
 * Generic Interrupt Controller: Redistributor (GICR)
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
    auto const map { reinterpret_cast<uintptr_t>(Hptp::map_tmp (phys, size, Paging::R, Memattr::dev(), 1)) };
    if (!map) [[unlikely]]
        return false;

    for (auto ptr { map }; ptr < map + size; ) {

        auto const type { *reinterpret_cast<uint64_t volatile *>(ptr + std::to_underlying (Reg64::TYPER)) };

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
    if (!Cpu::gicr) [[unlikely]]
        return false;

    // Map MMIO region
    if (Hptp::current().update (MMAP_CPU_GICR, Cpu::gicr, bit_scan_msb (mmio_size) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev()) != Status::SUCCESS) [[unlikely]]
        return false;

    auto const arch { Coresight::read (Coresight::Component::PIDR2, MMAP_CPU_GICR + 0x10000) >> 4 & BIT_RANGE (3, 0) };
    auto const iidr { read (Reg32::IIDR) };
    auto const type { read (Reg64::TYPER) };

    // Determine GICR processor number
    Cpu::gicr_num = static_cast<uint16_t>(type >> 8);

    // Disable to determine coherency support
    if (!set_ctlr (0)) [[unlikely]]
        return false;

    // Try programming desired cacheability/shareability attributes (requires CTLR.EnableLPIs == 0)
    write (Reg64::PROPBASER, attr_isic);
    write (Reg64::PENDBASER, attr_isic);

    // Treat GICR as non-coherent if cacheability/shareability bits are not programmable
    if (((read (Reg64::PROPBASER) ^ attr_isic) | (read (Reg64::PENDBASER) ^ attr_isic)) & impl_bits) [[unlikely]]
        coherent = false;

    if (Cpu::bsp) [[unlikely]] {

        // Determine number of EPPIs
        Interrupt::num_eppi = min (NUM_EPPI, static_cast<unsigned>(type >> 22 & BIT_RANGE (9, 5)));

        // Determine number of INTID bits
        idbits = bit_scan_msb (Interrupt::iid_msk + 1);
    }

    // Set up LPI tables
    if (type & BIT (0)) [[likely]] {

        // Global LPI configuration table
        if (Cpu::bsp) [[unlikely]]
            cfg_table = new (idbits, coherent) Cfg_table;

        // CPU-local LPI pending table
        pnd_table = new (idbits, coherent) Pnd_table;

        // Ensure alignment constraints
        assert (!(Kmem::ptr_to_phys (cfg_table) & BIT_RANGE (11, 0)));
        assert (!(Kmem::ptr_to_phys (pnd_table) & BIT_RANGE (15, 0)));
    }

    trace (TRACE_INTR, "GICR: %#010lx %03x:%03x r%up%u v%u EPPI:%u MPAM:%u DLPI:%u VLPI:%u PLPI:%u ID:%u (#%u)",
           Cpu::gicr, iidr & BIT_RANGE (11, 0), iidr >> 24, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0), arch,
           Interrupt::num_eppi, !!(type & BIT (6)), !!(type & BIT (3)), !!(type & BIT (1)), !!(type & BIT (0)), idbits, Cpu::gicr_num);

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
    init_intids (NUM_SGI + NUM_PPI, BASE_SGI);

    // Enable all SGIs
    write (Arr32::ISENABLER, 0, BIT_RANGE (15, 0));

    // Ensure required SGIs are available
    constexpr auto sgi { BIT (Interrupt::RRQ) | BIT (Interrupt::RKE) };
    if ((read (Arr32::ISENABLER, 0) & sgi) != sgi) [[unlikely]]
        return false;

    // Init EPPIs
    init_intids (Interrupt::num_eppi, BASE_EPPI - 1024);

    auto const type { read (Reg64::TYPER) };

    // Init LPIs if supported (requires CTLR.EnableLPIs == 0)
    if (type & BIT (0)) [[likely]] {

        // Ensure data structure allocations succeeded
        if (!cfg_table || !pnd_table || idbits < 14)
            return false;

        // Configure configuration and pending table
        write (Reg64::PROPBASER, (coherent ? attr_isic : attr_nsnc) | Kmem::ptr_to_phys (cfg_table) | (idbits - 1));
        write (Reg64::PENDBASER, (coherent ? attr_isic : attr_nsnc) | Kmem::ptr_to_phys (pnd_table));

        // Enable LPIs
        if (!set_ctlr (BIT (0))) [[unlikely]]
            return false;
    }

    // Wake CPU interface
    return set_sleep (false);
}

/*
 * Initialize INTID range
 *
 * @param n Number of INTIDs
 * @param o Offset in Register Block
 */
void Gicr::init_intids (unsigned n, unsigned o)
{
    // Assign interrupt groups and disable
    for (unsigned i { o }; i < o + n; i += 32) {
        write (Arr32::IGROUPR,   i / 32, Gicd::group);
        write (Arr32::ICENABLER, i / 32, BIT_RANGE (31, 0));
    }

    // Assign interrupt priorities
    for (unsigned i { o }; i < o + n; i += 4)
        write (Arr32::IPRIORITYR, i / 4, 0);
}

bool Gicr::act_get (unsigned iid)
{
    assert (Gicd::arch >= 3);

    // INTID must be a valid PPI or EPPI
    assert ((iid >= BASE_PPI && iid < Intid::from_ppi (Intid::NUM_PPI)) || (iid >= BASE_EPPI && iid < Intid::from_eppi (Interrupt::num_eppi)));

    // Adjust INTID for EPPI
    if (iid >= BASE_EPPI) [[unlikely]]
        iid -= 1024;

    // Determine active state
    return read (Arr32::ISACTIVER, iid / 32) & BIT (iid % 32);
}

void Gicr::act_set (unsigned iid, bool act)
{
    assert (Gicd::arch >= 3);

    // INTID must be a valid PPI or EPPI
    assert ((iid >= BASE_PPI && iid < Intid::from_ppi (Intid::NUM_PPI)) || (iid >= BASE_EPPI && iid < Intid::from_eppi (Interrupt::num_eppi)));

    // Adjust INTID for EPPI
    if (iid >= BASE_EPPI) [[unlikely]]
        iid -= 1024;

    // Select SET or CLR register
    auto const reg { act ? Arr32::ISACTIVER : Arr32::ICACTIVER };
    auto const num { iid / 32 };
    auto const msk { BIT (iid % 32) };

    // Configure active state
    write (reg, num, msk);

    Barrier::fsb (Barrier::Domain::NSH);
}

void Gicr::conf_ppi (unsigned iid, bool lvl, bool msk)
{
    assert (Gicd::arch >= 3);

    // INTID must be a valid PPI or EPPI
    assert ((iid >= BASE_PPI && iid < Intid::from_ppi (Intid::NUM_PPI)) || (iid >= BASE_EPPI && iid < Intid::from_eppi (Interrupt::num_eppi)));

    // Adjust INTID for EPPI
    if (iid >= BASE_EPPI) [[unlikely]]
        iid -= 1024;

    // Disable during reconfiguration
    write (Arr32::ICENABLER, iid / 32, BIT (iid % 32));

    // Wait for ICENABLER update to propagate throughout the affinity hierarchy
    wait_rwp();

    // Configure trigger mode
    auto const b { BIT (iid % 16 * 2 + 1) };
    auto const v { read (Arr32::ICFGR, iid / 16) };
    write (Arr32::ICFGR, iid / 16, lvl ? v & ~b : v | b);

    // Enable as needed
    if (!msk) [[likely]]
        write (Arr32::ISENABLER, iid / 32, BIT (iid % 32));
}

void Gicr::conf_lpi (unsigned iid, bool msk)
{
    assert (Gicd::arch >= 3);

    // INTID must be a valid LPI
    assert (iid >= BASE_LPI && iid < Intid::from_lpi (Interrupt::num_lpi));

    // Global LPI configuration table must exist
    assert (cfg_table);

    // Enable as needed
    cfg_table[Intid::to_lpi (iid)].set (msk, coherent);
}
