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

INIT_PRIORITY (PRIO_LOCAL) Atomic<Refptr<Sm>> Interrupt::sm_table[NUM_GSI];

void Interrupt::setup()
{
    Idt::build();
}

void Interrupt::rke_handler()
{
    if (Pd::current->Space_mem::htlb.tst (Cpu::id))
        Cpu::hazard |= Hazard::SCHED;
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

void Interrupt::handle_ipi (unsigned n)
{
    assert (n < NUM_IPI);

    Counter::req[n].inc();

    switch (n) {
        case Request::RRQ: Sc::rrq_handler(); break;
        case Request::RKE: rke_handler(); break;
    }
}

void Interrupt::handle_gsi (unsigned n)
{
    assert (n < NUM_GSI);

    // Atomic load because other CPUs can update the table concurrently
    Sm *const sm { sm_table[n].load() };

    if (sm) [[likely]]
        sm->up();
}

void Interrupt::handler (unsigned v)
{
    if (v == VEC_SVI)
        return;

    else if (v >= VEC_FLT)
        Smmu::all_interrupt();

    else if (v >= VEC_LVT)
        handle_lvt (v - VEC_LVT);

    else if (v >= VEC_IPI)
        handle_ipi (v - VEC_IPI);

    else if (v >= VEC_GSI)
        handle_gsi (v - VEC_GSI);

    Lapic::eoi();
}

void Interrupt::deactivate (Sm const *)
{
}

Status Interrupt::assign (bool attach, Sm * const sm, pci_t sbdf, uint16_t idx, uint16_t cpu, uint8_t vec, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // SM must be valid
    assert (sm);

    // CPU must be valid
    assert (cpu < Cpu::count);

    // VEC must be valid
    if (vec >= num_vec) [[unlikely]]
        return Status::BAD_PAR;

    // Convert interrupt to SEG:GSI
    Intid const iid { 0 };
    auto const seg { iid.seg() };
    auto const gsi { iid.gsi() };

    // Determine if PIN interrupt
    auto const ioapic { Ioapic::lookup (seg, gsi) };

    // Overwrite (PIN) or check (MSI) per-device IRT index (if IR)
    if (!Smmu::noir) {
        if (ioapic) [[unlikely]]
            idx = static_cast<uint16_t>(ioapic->get_pin (gsi));
        else if (idx >= num_idx) [[unlikely]]
            return Status::BAD_PAR;
    }

    // Source is IOAPIC (PIN) or Device (MSI)
    auto const src { ioapic ? ioapic->src() : sbdf };

    // Source and interrupt must be in the same PCI segment group
    if (Pci::seg (src) != seg) [[unlikely]]
        return Status::BAD_DEV;

    // Determine destination
    apic_t const dst { Lapic::id[cpu] };

    // Check DST/CFG (attach only)
    if (attach) [[likely]] {

        // DST is limited to 8-bit unless SMMUs operate in x2APIC mode with IR active
        if (dst >= (Smmu::x2apic && !Smmu::noir ? BIT_RANGE (31, 0) : BIT_RANGE (7, 0))) [[unlikely]]
            return Status::BAD_CPU;

        // Reserved CFG bits must be zero
        if (ioapic ? (cfg & ~BIT_RANGE (2, 0)) : cfg) [[unlikely]]
            return Status::BAD_PAR;
    }

    // Lookup SMMU for the PCI segment group (if IR)
    auto const smmu { Smmu::noir ? nullptr : Smmu::lookup_seg (seg) };
    if (Smmu::noir != !smmu) [[unlikely]]
        return Status::BAD_DEV;

    // Lookup SMMU IRTE for the interrupt (if IR)
    Atomic<Smmu::Entry> *irte { nullptr };
    if (!Smmu::noir) [[likely]]
        if (auto const s { smmu->irte_get (irte, iid, src, idx, attach) }; s != Status::SUCCESS) [[unlikely]]
            return s;

    // Determine SM table slot
    auto const ptr { Kmem::loc_to_glb (cpu, sm_table + vec) };

    // Promote SM table index to absolute vector number
    vec += VEC_GSI;

    // Serialize multi-step updates per interrupt semaphore
    Lock_guard <Spinlock> guard { sm->lock };

    Atomic<uintptr_t> ise;

    if (attach) [[likely]] {

        {   // Update SM table: nullptr -> sm
            Sm *old { nullptr }; Refptr<Sm> ref { sm };

            // Failed to acquire reference on sm
            if (ref != sm) [[unlikely]]
                return Status::ABORTED;

            // Try atomic attach and permit reattach
            if (!ptr->compare_exchange (old, ref)) [[unlikely]]
                if (old != sm) [[unlikely]]
                    return Status::ABORTED;

            // Scope exit drops ref on sm (reattach) or nullptr (attach)
            assert (ref == sm || ref == nullptr);
        }

        // Extract configuration from flags
        bool const msk { !!(cfg & BIT (0)) };
        bool const trg { !!(cfg & BIT (1)) };
        bool const pol { !!(cfg & BIT (2)) };

        // Update SMMU IRTE (if IR)
        if (!Smmu::noir) [[likely]]
            if (auto const s { smmu->irte_set (irte, ise, ioapic, iid, src, dst, vec, trg, Smmu::ise_msi (src, idx)) }; s != Status::SUCCESS) [[unlikely]]
                return s;

        // Helper lambda for MSI return values: both default to 0 (PIN), overridden by each MSI path
        auto const set_msi { (msi_addr = msi_data = 0, [&] (uintptr_t addr, uintptr_t data) { msi_addr = addr; msi_data = data; }) };

        // Update IOAPIC RTE (if PIN)
        if (Smmu::noir) [[unlikely]]
            ioapic ? ioapic->rte_set_compat (ise, gsi, msk, trg, pol, static_cast<uint8_t>(dst), vec) : set_msi (Lapic::msi_base | dst << 12, vec);
        else if (Smmu::type() == Smmu::Type::AMD)
            ioapic ? ioapic->rte_set_ir_amd (ise, gsi, msk, trg, pol) : set_msi (Lapic::msi_base, idx);
        else if (Smmu::type() == Smmu::Type::ITL)
            ioapic ? ioapic->rte_set_ir_itl (ise, gsi, msk, trg, pol) : set_msi (Lapic::msi_base | BIT_RANGE (4, 3), gsi);
        else
            __builtin_unreachable();

        trace (TRACE_INTR, "INTR: Routing GSI %#06x (%c%c%c) from %04x:%02x:%02x.%x to %#06x:%#04x (%s)", gsi, msk ? 'M' : 'U', trg ? 'L' : 'E', pol ? 'L' : 'H', Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src), cpu, vec, ioapic ? "PIN" : "MSI");

    } else {

        // Update IOAPIC RTE (if PIN)
        if (Smmu::noir && ioapic)
            ioapic->rte_clr_compat (ise, gsi, static_cast<uint8_t>(dst), vec);

        // Update SMMU IRTE (if IR)
        if (!Smmu::noir) [[likely]]
             if (auto const s { smmu->irte_clr (irte, ise, ioapic, iid, src, dst, vec) }; s != Status::SUCCESS) [[unlikely]]
                 return s;

        {   // Update SM table: sm -> nullptr
            Sm *old { sm }; Refptr<Sm> ref;

            // Try atomic detach
            if (!ptr->compare_exchange (old, ref)) [[unlikely]]
                return Status::ABORTED;

            // Scope exit drops ref on sm (detach)
            assert (ref == sm);
        }
    }

    return Status::SUCCESS;
}

void Interrupt::send_cpu (Request req, cpu_t cpu)
{
    Lapic::send_cpu (VEC_IPI + req, cpu);
}

void Interrupt::send_exc (Request req)
{
    Lapic::send_exc (VEC_IPI + req);
}
