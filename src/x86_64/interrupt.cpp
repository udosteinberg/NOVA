/*
 * Interrupt Handling
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "counter.hpp"
#include "idt.hpp"
#include "interrupt.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include "sm.hpp"
#include "smmu.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

Interrupt Interrupt::int_table[NUM_GSI];

void Interrupt::setup()
{
    Idt::build();

    Status s;

    for (unsigned gsi { 0 }; gsi < sizeof (int_table) / sizeof (*int_table); gsi++)
        int_table[gsi].sm = Pd::create_sm (s, &Space_obj::nova, Space_obj::Selector::NOVA_INT + gsi, 0, gsi);
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
        Cpu::hazard |= Hazard::SLEEP;

    if (Space_hst::current->htlb.tst (Cpu::id))
        Cpu::hazard |= Hazard::SCHED;
}

void Interrupt::handle_ipi (unsigned ipi)
{
    assert (ipi < NUM_IPI);

    Counter::req[ipi].inc();

    switch (ipi) {
        case Request::RRQ: Scheduler::requeue(); break;
        case Request::RKE: rke_handler(); break;
    }
}

void Interrupt::handle_lvt (unsigned lvt)
{
    assert (lvt < NUM_LVT);

    Counter::loc[lvt].inc();

    switch (lvt) {
        case 0: Lapic::timer_handler(); break;
        case 1: Lapic::error_handler(); break;
        case 2: Lapic::perfm_handler(); break;
        case 3: Lapic::therm_handler(); break;
    }
}

void Interrupt::handle_gsi (unsigned gsi)
{
    assert (gsi < NUM_GSI);

    set_mask (gsi, true);

    int_table[gsi].sm->up();
}

void Interrupt::handler (unsigned v)
{
    if (v >= VEC_FLT)
        Smmu::interrupt();

    else if (v >= VEC_IPI)
        handle_ipi (v - VEC_IPI);

    else if (v >= VEC_LVT)
        handle_lvt (v - VEC_LVT);

    else if (v >= VEC_GSI)
        handle_gsi (v - VEC_GSI);

    Lapic::eoi();
}

void Interrupt::init (unsigned gsi, uint32_t &msi_addr, uint16_t &msi_data)
{
    Config cfg  = int_table[gsi].config;
    auto ioapic = int_table[gsi].ioapic;

    auto rid = ioapic ? ioapic->rid() : cfg.rid();
    auto aid = Lapic::id[cfg.cpu()];
    auto vec = static_cast<uint8_t>(VEC_GSI + gsi);

    Smmu::set_irte (static_cast<uint16_t>(gsi), rid, aid, vec, cfg.trg());

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
        ioapic->set_dst (gsi, Smmu::ir ? gsi << 17 | BIT (16) : aid << 24);
        ioapic->set_cfg (gsi, cfg.msk(), cfg.trg(), cfg.pol());
        msi_addr = msi_data = 0;
    } else {
        msi_addr = 0xfee << 20 | (Smmu::ir ? BIT_RANGE (4, 3) : aid << 12);
        msi_data = static_cast<uint16_t>(Smmu::ir ? gsi : vec);
    }
}

void Interrupt::configure (unsigned gsi, Config cfg, uint32_t &msi_addr, uint16_t &msi_data)
{
    trace (TRACE_INTR, "INTR: %s: %u cpu=%u %c%c%c", __func__, gsi, cfg.cpu(), cfg.msk() ? 'M' : 'U', cfg.trg() ? 'L' : 'E', cfg.pol() ? 'L' : 'H');

    int_table[gsi].config = cfg;

    init (gsi, msi_addr, msi_data);
}

void Interrupt::send_cpu (Request req, cpu_t cpu)
{
    Lapic::send_cpu (VEC_IPI + req, cpu);
}

void Interrupt::send_exc (Request req)
{
    Lapic::send_exc (VEC_IPI + req);
}
