/*
 * GIC Interrupt Translation Service (GITS)
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

#include "gits.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Gits::cache { sizeof (Gits), alignof (Gits) };

Gits::Gits (uint64_t p, uint32_t i) : List { list }, Mmio { p, 0x10000, Memattr::dev() }, id { i }, typer { read (Reg64::TYPER) }
{
    // GICv3 spec requires registers to be 64K aligned. The page containing GITS_TRANSLATER must be accessible to userland
    assert ((p & ALIGNMENT_OFFS (16)) == 0);

    // GICv3 spec requires physical LPI support to be present
    assert (feat_plpi());

    // Disable to determine coherency support
    if (!set_ctlr (false)) [[unlikely]]
        panic ("GITS disable failed");

    // Try programming desired cacheability/shareability attributes
    write (Reg64::CBASER, attr_isic);

    // Treat as non-coherent if cacheability/shareability bits are not programmable
    if ((read (Reg64::CBASER) ^ attr_isic) & impl_bits) [[unlikely]]
        coherent = false;

    // Enumerate ITS translation table descriptors
    for (unsigned n { 0 }; n < 8; n++)
        if (!init_baser (n)) [[unlikely]]
            panic ("GITS BASER failed");

    auto const arch { Coresight::read (Coresight::Component::PIDR2, mmio + 0x10000) >> 4 & BIT_RANGE (3, 0) };
    auto const iidr { read (Reg32::IIDR) };

    trace (TRACE_INTR, "GITS: %#010lx %03x:%03x r%up%u v%u DEV:%u EVT:%u ITT:%u HCC:%u VLPI:%u PLPI:%u C:%u (#%u)",
           phys, BIT_RANGE (11, 0) & iidr, iidr >> 24, BIT_RANGE (3, 0) & iidr >> 16, BIT_RANGE (3, 0) & iidr >> 12, arch,
           ord_dev(), ord_evt(), ord_itt(), num_hcc(), feat_vlpi(), feat_plpi(), coherent, id);
}

bool Gits::init_baser (unsigned n)
{
    // CPU count must be known
    assert (Cpu::count);

    // Try programming desired cacheability/shareability attributes and the indirect bit
    write (Arr64::BASER, n, BIT64 (62) | attr_isic);

    // Read the register back
    auto const val { read (Arr64::BASER, n) };

    uint64_t *baser;
    unsigned ord;

    // Determine table type
    switch (val >> 56 & BIT_RANGE (2, 0)) {

        case 1:     // DEV Table
            baser = &baser_dev;
            ord = ord_dev();
            break;

        case 4:     // COL Table
            baser = &baser_col;
            ord = bit_scan_msb (Cpu::count - 1) + 1;
            break;

        default:    // Ignore unimplemented or irrelevant BASER
            return true;
    }

    // Treat as non-coherent if cacheability/shareability bits are not programmable
    if ((val ^ attr_isic) & impl_bits) [[unlikely]]
        coherent = false;

    // Determine indirect support and entry size
    auto const i { !!(val & BIT64 (62)) };
    auto const e { (val >> 48 & BIT_RANGE (4, 0)) + 1 };

    // Try possible page encodings: 0 (4K), 1 (16K), 2 (64K)
    for (unsigned p { 0 }; p < 3; p++) {

        // Convert page encoding to page size
        auto const s { PAGE_SIZE (0) << p * 2 };

        // A maximum of 256 pages (order 8) can be concatenated via BASER[7:0]
        constexpr auto max_c { 8 };

        // Compute level orders
        auto const ord_0 { static_cast<unsigned>(bit_scan_msb (s / e)) };
        auto const ord_1 { ord > max_c + ord_0 && i ? static_cast<unsigned>(bit_scan_msb (s / sizeof (Table))) : 0 };
        auto const ord_c { ord > ord_1 + ord_0 ? ord - ord_1 - ord_0 : 0 };

        // Fail if we need more concatenated pages than possible
        if (ord_c > max_c) [[unlikely]]
            continue;

        auto const size { p << 8 | (BIT (ord_c) - 1) };

        // Try programming desired page encoding and number of concatenated pages
        write (Arr64::BASER, n, size);

        // BASER uses a fixed page size that differs from our choice
        if ((read (Arr64::BASER, n) ^ size) & BIT64_RANGE (9, 0)) [[unlikely]]
            continue;

        // Allocate (concatenated) root table
        auto const ptr { new (ord_c + p * 2, coherent) Table };

        // Allocation failure
        if (!ptr) [[unlikely]]
            break;

        // Determine final BASER value and use read-only bits[58:56] to store the BASER number
        *baser = BIT64 (63) | BIT64 (62) * !!ord_1 | (coherent ? attr_isic : attr_nsnc) | uint64_t { n } << 56 | (val & BIT64_RANGE (52, 48)) | Kmem::ptr_to_phys (ptr) | size;

        trace (TRACE_INTR, "GITS: %#010lx BASER%u:%#lx E:%lu I:%u O:%x%x%x S:%#lx", phys, n, *baser, e, i, ord_c, ord_1, ord_0, s);

        return true;
    }

    return false;
}

bool Gits::init_table (uint64_t baser, unsigned idx)
{
    // BASER must be valid
    if (!(baser & BIT64 (63))) [[unlikely]]
        return false;

    // A flat table has already been fully allocated
    if (!(baser & BIT64 (62))) [[unlikely]]
        return true;

    // For a two-level table, determine entry size, page order and number of concatenated pages
    auto const e { static_cast<unsigned>(baser >> 48 & BIT_RANGE (4, 0)) + 1 };
    auto const o { static_cast<unsigned>(baser >>  8 & BIT_RANGE (1, 0)) * 2 };
    auto const n { static_cast<unsigned>(baser       & BIT_RANGE (7, 0)) + 1 };

    // Compute top-level slot number
    auto const s { idx / ((PAGE_SIZE (0) << o) / e) };

    // Fail if slot exceeds the number of top-level slots
    if (s >= (PAGE_SIZE (0) << o) / sizeof (Table) * n) [[unlikely]]
        return false;

    // Try to allocate the leaf table for that slot
    return static_cast<Table *>(Kmem::phys_to_ptr (baser & BIT64_RANGE (47, 12)))[s].allocate (o, coherent);
}

bool Gits::command (Cmd const &c)
{
    Lock_guard <Spinlock> guard { cmdq.lock };

    // Add command. The queue is normally empty, unless we have a stalled command replaced by SYNC in it
    cmdq.produce (c, coherent);

    // Notify GITS. We always set the retry bit to recover from a potential earlier stall
    write (Reg64::CWRITER, cmdq.get_idx() << 5 | BIT (0));

    uint32_t hwi; bool err;

    // Fail if the command timed out without stalling or completing
    if (!Wait::until (timeout, [&] { auto const v { read (Reg64::CREADR) }; err = v & BIT (0); hwi = v >> 5 & BIT_RANGE (14, 0); return err || cmdq.is_empty (hwi); })) [[unlikely]]
        return false;

    // Fail if the command stalled and replace it with a SYNC command
    if (err) [[unlikely]] {
        Cmd e { Cmd_sync { rta (Cpu::id) } };
        cmdq.replace (e, coherent, hwi);
        trace (TRACE_ERROR, "GITS: %#010lx Command failed %#lx %#lx %#lx %#lx", phys, uint64_t { e.w0 }, uint64_t { e.w1 }, uint64_t { e.w2 }, uint64_t { e.w3 });
        return false;
    }

    return true;
}

bool Gits::init()
{
    // Disable GITS to facilitate writing CBASER and BASER[n]
    if (!set_ctlr (false)) [[unlikely]]
        return false;

    // Ensure CMD queue is allocated and 64K aligned
    if (!cmdq.ptr || cmdq.get_ptr() & ALIGNMENT_OFFS (16)) [[unlikely]]
        return false;

    // Configure CMD queue (CBASER write sets CREADR to 0)
    write (Reg64::CBASER, (coherent ? attr_isic : attr_nsnc) | BIT64 (63) | cmdq.get_ptr() | (BIT (cmdq.pao) - 1));
    write (Reg64::CWRITER, 0);

    // Configure DEV table
    if (baser_dev) [[likely]]
        write (Arr64::BASER, static_cast<unsigned>(baser_dev >> 56 & BIT_RANGE (2, 0)), baser_dev);

    // Configure COL table
    if (baser_col) [[likely]]
        write (Arr64::BASER, static_cast<unsigned>(baser_col >> 56 & BIT_RANGE (2, 0)), baser_col);

    // Enable GITS once CBASER and BASER[n] are valid
    if (!set_ctlr (true)) [[unlikely]]
        return false;

    // Establish collection table entries for all CPUs
    for (arm_colid_t cpu { 0 }; cpu < Cpu::count; cpu++) {

        // If CPU count exceeds HCC, then ensure collection table entry exists
        if (Cpu::count > num_hcc() && !init_table (baser_col, cpu)) [[unlikely]]
            return false;

        trace (TRACE_INTR | TRACE_PARSE, "GITS: %#010lx MAPC CID:%u => RTA:%#lx", phys, cpu, rta (cpu));

        // Map CID => GICR and invalidate collection cache
        if (!command (Cmd_mapc { cpu, rta (cpu) }) || !command (Cmd_invall { cpu })) [[unlikely]]
            return false;
    }

    return true;
}

Status Gits::conf_lpi (arm_intid_t iid, arm_colid_t cid, arm_devid_t did, arm_evtid_t eid, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // Fail if DID or EID is too large
    if (did >= BIT (ord_dev()) || eid >= BIT (ord_itt())) [[unlikely]]
        return Status::BAD_PAR;

    trace (TRACE_INTR | TRACE_PARSE, "GITS: %#010lx MAPTI DID:%#x EID:%u => IID:%u CID:%u", phys, did, eid, iid, cid);

    // Map DID:EID => IID:CID and ensure subsequent interrupt translations honor the updated state
    if (!command (Cmd_mapti { did, eid, iid, cid }) || !command (Cmd_sync { rta (cid) })) [[unlikely]]
        return Status::ABORTED;

    // Set MSI target
    msi_addr = phys + std::to_underlying (Reg32::TRANSLATER);
    msi_data = eid;

    return Status::SUCCESS;
}
