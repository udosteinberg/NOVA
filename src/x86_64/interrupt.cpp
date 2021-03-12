/*
 * Interrupt Handling
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "acpi.hpp"
#include "counter.hpp"
#include "idt.hpp"
#include "interrupt.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include "sm.hpp"
#include "smmu.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_LOCAL) Refptr<Sm> Interrupt::sm_table[NUM_GSI];

void Interrupt::setup()
{
    Idt::build();
}

void Interrupt::rke_handler()
{
    if (Acpi::get_transition().valid())
        Cpu::hazard |= Hazard::SLEEP;

    if (Space_hst::current->htlb.tst (Cpu::id))
        Cpu::hazard |= Hazard::SCHED;
}

void Interrupt::handle_ipi (unsigned n)
{
    assert (n < NUM_IPI);

    Counter::req[n].inc();

    switch (n) {
        case Request::RRQ: Sc::rrq_handler(); break;
        case Request::RKE: rke_handler(); break;
    }
}

void Interrupt::handle_lvt (unsigned n)
{
    assert (n < NUM_LVT);

    Counter::loc[n].inc();

    switch (n) {
        case 0: Lapic::handle_timer(); break;
        case 1: Lapic::handle_error(); break;
        case 2: Lapic::handle_perfm(); break;
        case 3: Lapic::handle_therm(); break;
        case 4: Lapic::handle_cmchk(); break;
    }
}

void Interrupt::handle_gsi (unsigned n)
{
    assert (n < NUM_GSI);

    // Atomic load because other CPUs can update the table concurrently
    Sm *const sm { sm_table[n].atomic_load() };

    if (sm) [[likely]]
        sm->up();
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

void Interrupt::deactivate (gsi_t gsi)
{
    uint8_t vec;

    // Level-triggered interrupts require directed EOI to IOAPIC
    if (Smmu::gsi_lookup (gsi, vec)) [[unlikely]] {

        auto const ioapic { Ioapic::lookup (gsi) };

        // Level-triggered implies PIN => IOAPIC must exist
        assert (ioapic);

        ioapic->eoi (vec);
    }
}

Status Interrupt::assign (Sm *sm, cpu_t cpu, gsi_t vec, pci_t src, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // Table index must be valid
    if (vec > vec_max) [[unlikely]]
        return Status::BAD_PAR;

    {   Refptr<Sm> ref { sm };

        // Failed to acquire reference
        if (ref != sm) [[unlikely]]
            return Status::ABORTED;

        // Atomically replace Refptr<Sm> in the CPU's table
        Kmem::loc_to_glb (cpu, sm_table + vec)->atomic_swap (ref);
    }

    // Detach
    if (!sm) [[unlikely]]
        return Status::SUCCESS;

    // Attach
    return Smmu::assign_int (0, cpu, static_cast<uint8_t>(VEC_GSI + vec), src, cfg, msi_addr, msi_data);
}

void Interrupt::send_cpu (Request req, cpu_t cpu)
{
    Lapic::send_cpu (VEC_IPI + req, cpu);
}

void Interrupt::send_exc (Request req)
{
    Lapic::send_exc (VEC_IPI + req);
}
