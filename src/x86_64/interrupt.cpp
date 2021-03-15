/*
 * Interrupt Handling
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "counter.hpp"
#include "hazards.hpp"
#include "idt.hpp"
#include "interrupt.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include "pd_kern.hpp"
#include "sm.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

Interrupt Interrupt::int_table[NUM_GSI];

void Interrupt::setup()
{
    Idt::build();

    for (unsigned gsi = 0; gsi < sizeof (int_table) / sizeof (*int_table); gsi++)
        int_table[gsi].sm = Sm::create (0, gsi);
}

void Interrupt::set_mask (unsigned gsi, bool msk)
{
    Config cfg  = int_table[gsi].config;
    auto ioapic = int_table[gsi].ioapic;

    if (ioapic && cfg.trg())
        ioapic->set_cfg (gsi, msk, true, cfg.pol());
}

void Interrupt::rke_handler()
{
    if (Acpi::get_transition().state())
        Cpu::hazard |= HZD_SLEEP;

    if (Pd::current->Space_mem::htlb.tst (Cpu::id))
        Cpu::hazard |= HZD_SCHED;
}

void Interrupt::handle_ipi (unsigned vector)
{
    unsigned ipi = vector - VEC_IPI;

    Counter::req[ipi].inc();

    switch (ipi) {
        case Request::RRQ: Scheduler::requeue(); break;
        case Request::RKE: rke_handler(); break;
    }

    Lapic::eoi();
}

void Interrupt::handle_lvt (unsigned vector)
{
    unsigned lvt = vector - VEC_LVT;

    Counter::loc[lvt].inc();

    switch (vector) {
        case VEC_LVT_TIMER: Lapic::timer_handler(); break;
        case VEC_LVT_ERROR: Lapic::error_handler(); break;
        case VEC_LVT_PERFM: Lapic::perfm_handler(); break;
        case VEC_LVT_THERM: Lapic::therm_handler(); break;
    }

    Lapic::eoi();
}

void Interrupt::handle_gsi (unsigned vector)
{
    unsigned gsi = vector - VEC_GSI;

    Counter::gsi[gsi].inc();

    set_mask (gsi, true);

    Lapic::eoi();

    int_table[gsi].sm->up();
}

void Interrupt::init (unsigned gsi, uint32 &msi_addr, uint16 &msi_data)
{
    Config cfg  = int_table[gsi].config;
    auto ioapic = int_table[gsi].ioapic;

    auto rid = ioapic ? ioapic->rid() : cfg.rid();
    auto aid = Cpu::apic_id[cfg.cpu()];
    auto vec = static_cast<uint8>(VEC_GSI + gsi);

    Smmu::set_irt (gsi, rid, aid, vec, cfg.trg());

    /* MSI Compatibility Format
     * ADDR: 0xfee[31:20] APICID[19:12] ---[11:5] 0[4] RH[3] DM[2] --[1:0]
     * DATA: ---[31:16] TRG[15] ASS[14] --[13:11] DLVM[10:8] VEC[7:0]
     *
     * MSI Remappable Format
     * ADDR: 0xfee[31:20] Handle[19:5] 1[4] SHV[3] Handle[2] --[1:0]
     * DATA: ---[31:16] Subhandle[15:0]
     */

    /* IOAPIC RTE Format
     * 63:56/31:24 = APICID     => ADDR[19:12]
     * 55:48/23:16 = EDID       => ADDR[11:4]
     * 16 = MSK
     * 15 = TRG                 => DATA[15]
     * 14 = RIRR
     * 13 = POL
     * 12 = DS
     * 11 = DM                  => ADDR[2]
     * 10:8 = DLV               => DATA[10:8]
     * 7:0 = VEC                => DATA[7:0]
     */

    if (ioapic) {
        ioapic->set_dst (gsi, Smmu::use_ir ? gsi << 17 | BIT (16) : aid << 24);
        ioapic->set_cfg (gsi, cfg.msk(), cfg.trg(), cfg.pol());
        msi_addr = msi_data = 0;
    } else {
        msi_addr = 0xfee << 20 | (Smmu::use_ir ? BIT_RANGE (4, 3) : aid << 12);
        msi_data = static_cast<uint16>(Smmu::use_ir ? gsi : vec);
    }
}

void Interrupt::configure (unsigned gsi, uint16 cpu, uint16 rid, uint8 flags, uint32 &msi_addr, uint16 &msi_data)
{
    trace (TRACE_INTR, "INTR: %s: %u cpu=%u %c%c%c", __func__, gsi, cpu, flags & BIT (0) ? 'M' : 'U', flags & BIT (1) ? 'L' : 'E', flags & BIT (2) ? 'L' : 'H');

    int_table[gsi].config = Config (cpu, rid, flags);

    init (gsi, msi_addr, msi_data);
}

void Interrupt::send_cpu (Request req, unsigned cpu)
{
    Lapic::send_cpu (VEC_IPI + req, cpu);
}

void Interrupt::send_exc (Request req)
{
    Lapic::send_exc (VEC_IPI + req);
}
