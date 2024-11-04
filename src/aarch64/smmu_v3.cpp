/*
 * System Memory Management Unit (Arm SMMUv3)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "interrupt.hpp"
#include "smmu_v3.hpp"
#include "space_dma.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu_v3::cache { sizeof (Smmu_v3), alignof (Smmu_v3) };

// SMMU registers occupy two consecutive 64KB pages starting from an at least 64KB-aligned boundary
Smmu_v3::Smmu_v3 (uint64_t base, unsigned e, unsigned p, unsigned g, unsigned s) : Smmu { base, 0x20000 }, spi { e, p, g, s },
                  idr0 { read (Reg32::IDR0) }, idr1 { read (Reg32::IDR1) }, idr5 { read (Reg32::IDR5) },
                  cmdq { new (sup_coh()) Fifo<Cmd>, hwo_cmd() },
                  evtq { new (sup_coh()) Fifo<Evt>, hwo_evt() }
{
    auto const c { sup_coh() };

    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt::noncoherent |= !c;

    trace (TRACE_SMMU, "SMMU: %#lx v3.%u CMD:%u EVT:%u SID:%u FMT:%u OAS:%u C:%u S1:%u S2:%u 32:%u 64:%u 4K:%u SPI:%#x/%#x/%#x/%#x",
           phys, read (Reg32::AIDR) & BIT_RANGE (7, 0),
           cmdq.ord, evtq.ord, hwo_sid(), str_fmt(), Dpt::pas (oas()),
           sup_coh(), sup_s1p(), sup_s2p(), sup_t32(), sup_t64(), sup_g04(),
           spi[0], spi[1], spi[2], spi[3]);

    // Allocate top-level stream table
    if (str_fmt()) [[likely]] {
        assert (str_lvl() > 0);
        ptr_str = new (c, hwo_sid()) Std;
    } else
        ptr_str = new (c) Ste;
}

Smmu_v3 *Smmu_v3::setup (uint64_t base, unsigned spi_e, unsigned spi_p, unsigned spi_g, unsigned spi_s)
{
    // Allocate SMMU
    return new Smmu_v3 { base, spi_e, spi_p, spi_g, spi_s };
}

bool Smmu_v3::init()
{
    for (unsigned i { 0 }; i < sizeof (spi) / sizeof (*spi); i++)
        if (spi[i] != ~0U) [[likely]]
            Interrupt::conf_spi (spi[i], false, false, Cpu::id);

    if (!sup_s2p()) [[unlikely]]
        return false;

    if (!sup_t64()) [[unlikely]]
        return false;

    if (!sup_g04()) [[unlikely]]
        return false;

    if (idr1 & BIT_RANGE (30, 29)) [[unlikely]]
        return false;

    // Configure global bypass to abort all incoming transactions
    if (!set_gbpa (BIT (20))) [[unlikely]]
        return false;

    // Acknowledge all pre-existing error conditions
    write (Reg32::GERRORN, read (Reg32::GERROR));

    if (sup_msi()) {

        // Disable GERROR MSI (requires IRQ_CTRL.BIT[0] == 0)
        write (Reg64::GERROR_IRQ_CFG0, 0);      // MSI Addr
        write (Reg32::GERROR_IRQ_CFG1, 0);      // MSI Data
        write (Reg32::GERROR_IRQ_CFG2, 0);      // Memory Type, Shareability

        // Disable EVENTQ MSI (requires IRQ_CTRL.BIT[0] == 0)
        write (Reg64::EVENTQ_IRQ_CFG0, 0);      // MSI Addr
        write (Reg32::EVENTQ_IRQ_CFG1, 0);      // MSI Data
        write (Reg32::EVENTQ_IRQ_CFG2, 0);      // Memory Type, Shareability
    }

    // Enable supported interrupts
    if (!set_irq_ctrl (BIT (2) | sup_pri() * BIT (1) | BIT (0))) [[unlikely]]
        return false;

    /*
     * SMMU_CR0.SMMUEN == 0 is required here to
     * - have a writable Reg64::STRTAB_BASE
     * - have a writable Reg32::STRTAB_BASE_CFG
     * - have a writable Reg32::CR2
     * - have a writable Reg32::CR1
     * - ensure all stream table structures are initially unreachable
     */

    // Configure stream table: read-allocate hint | base address, format | split point | order
    assert (ptr_str);
    write (Reg64::STRTAB_BASE, BIT64 (62) | Kmem::ptr_to_phys (ptr_str));
    write (Reg32::STRTAB_BASE_CFG, str_fmt() << 16 | ord_ste << 6 | hwo_sid());

    // Configure commmand queue: read-allocate hint | base address | order
    assert (cmdq.ptr);
    write (Reg64::CMDQ_BASE, BIT64 (62) | cmdq.get_base() | cmdq.ord);
    write (Reg32::CMDQ_PROD, 0);
    write (Reg32::CMDQ_CONS, 0);

    // Configure event queue: write-allocate hint | base address | order
    assert (evtq.ptr);
    write (Reg64::EVENTQ_BASE, BIT64 (62) | evtq.get_base() | evtq.ord);
    write (Reg32::EVENTQ_PROD, 0);
    write (Reg32::EVENTQ_CONS, 0);

    // Configure CR2: PTM | RECINVSID
    write (Reg32::CR2, BIT_RANGE (2, 1));

    // Configure CR1: TABLE_SH = IS | TABLE_OC = TABLE_IC = WB | QUEUE_SH = IS | QUEUE_OC = QUEUE_IC = WB
    write (Reg32::CR1, BIT_RANGE (11, 10) | BIT (8) | BIT (6) | BIT_RANGE (5, 4) | BIT (2) | BIT (0));

    // Enable CMDQEN | EVENTQEN
    if (!set_cr0 (BIT_RANGE (3, 2))) [[unlikely]]
        return false;

    // Invalidate cached configuration and EL2/EL1 TLB entries (see 3.11)
    if (!cfg_invalidate_all() || !tlb_invalidate_alle2() || !tlb_invalidate_alle1()) [[unlikely]]
        return false;

    // Enable CMDQEN | EVENTQEN | SMMUEN
    return set_cr0 (BIT_RANGE (3, 2) | BIT (0));
}

void Smmu_v3::fault()
{
    auto const err { read (Reg32::GERROR) };
    auto const ack { read (Reg32::GERRORN) };

    auto const e { err ^ ack };

    if (e & BIT (0)) [[unlikely]]       // CMDQ_ERR (see 7.1)
        trace (TRACE_SMMU, "SMMU: %#lx Command Execution Error %#x", phys, read (Reg32::CMDQ_CONS) >> 24);
    else
        trace (TRACE_SMMU, "SMMU: %#lx, Generic Error %#x", phys, e);

//  write (Reg32::GERRORN, err);
}
