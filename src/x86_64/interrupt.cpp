/*
 * Interrupt Handling
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "counter.hpp"
#include "idt.hpp"
#include "interrupt.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include "sm.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_LOCAL) Refptr<Sm> Interrupt::sm_table[NUM_GSI];

void Interrupt::setup()
{
    Idt::build();
}

void Interrupt::rke_handler()
{
    if (Pd::current->Space_hst::htlb.tst (Cpu::id))
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

void Interrupt::deactivate (Sm *)
{
    uint8_t vec;

    auto const gsi { 0 };

    auto const irt { Smmu::Grp::lookup_irt (gsi) };

    // Level-triggered interrupts require directed EOI to IOAPIC
    if (irt->lookup (vec)) [[unlikely]] {

        auto const ioapic { Ioapic::lookup (gsi) };

        // Level-triggered implies PIN => IOAPIC must exist
        assert (ioapic);

        ioapic->eoi (vec);
    }
}

Status Interrupt::assign (bool attach, Sm * const sm, cpu_t cpu, uint16_t vec, pci_t src, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // Semaphore must be valid
    assert (sm);

    // Table index must be valid
    if (vec >= num_vec) [[unlikely]]
        return Status::BAD_PAR;

    // Determine slot pointer
    auto const ptr { Kmem::loc_to_glb (cpu, sm_table + vec) };

    // Pointer must be valid
    assert (ptr);

    if (attach) [[likely]] {

        // Attach[ptr] nullptr -> sm
        Sm *old { nullptr }; Refptr<Sm> ref { sm };

        // Failed to acquire reference on sm
        if (ref != sm) [[unlikely]]
            return Status::ABORTED;

        // Try atomic attach or detect reattach
        if (!ptr->atomic_compare_exchange (old, ref)) [[unlikely]]
            if (old != sm) [[unlikely]]
                return Status::ABORTED;

        // Scope exit drops ref on sm (reattach) or nullptr (attach)
        assert (ref == sm || ref == nullptr);

    } else {

        // Detach[ptr] sm -> nullptr
        Sm *old { sm }; Refptr<Sm> ref { nullptr };

        // Try atomic detach
        if (!ptr->atomic_compare_exchange (old, ref)) [[unlikely]]
            return Status::ABORTED;

        // Scope exit drops ref on sm (detach)
        assert (ref == sm);

        return Status::SUCCESS;
    }

    auto const irt { Smmu::Grp::lookup_irt (0) };

    return Smmu::assign_int (irt, 0, cpu, static_cast<uint8_t>(VEC_GSI + vec), src, cfg, msi_addr, msi_data);
}

void Interrupt::send_cpu (Request req, cpu_t cpu)
{
    Lapic::send_cpu (VEC_IPI + req, cpu);
}

void Interrupt::send_exc (Request req)
{
    Lapic::send_exc (VEC_IPI + req);
}
