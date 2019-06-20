/*
 * Interrupt Handling
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
#include "counter.hpp"
#include "dc.hpp"
#include "gicc.hpp"
#include "gicd.hpp"
#include "gicr.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"

void Interrupt::rke_handler()
{
    if (Acpi::get_transition().valid())
        Cpu::hazard |= Hazard::SLEEP;
}

Event::Selector Interrupt::handle_sgi (unsigned n, auto const &dir)
{
    assert (n < NUM_SGI);

    Counter::req[n].inc();

    switch (n) {
        case Request::RRQ: Scheduler::requeue(); break;
        case Request::RKE: rke_handler(); break;
    }

    dir();

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_ppi (unsigned n, auto const &dir, bool vcpu)
{
    assert (n < NUM_PPI);

    Counter::loc[n].inc();

    if (n == Timer::ppi_el1_v)      // Deactivation by guest
        return vcpu ? Event::Selector::VTIMER : Event::Selector::NONE;

    if (n == Timer::ppi_el2_p)      // Deactivation by host
        Timeout::check();

    dir();

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_spi (unsigned n, auto const &dir)
{
    assert (n < num_spi);

    // Atomic load because other CPUs can update the table concurrently
    Sm *const sm { table_s[n].atomic_load() };

    if (sm) [[likely]]
        sm->up();

    else if (Smmu::using_iid (Intid::from_spi (n))) {

        Smmu::interrupt (Intid::from_spi (n));

        dir();
    }

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_eppi (unsigned n, auto const &dir)
{
    assert (n < num_eppi);

    dir();

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_espi (unsigned n, auto const &)
{
    assert (n < num_espi);

    // Atomic load because other CPUs can update the table concurrently
    Sm *const sm { table_e[n].atomic_load() };

    if (sm) [[likely]]
        sm->up();

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_lpi (unsigned n)
{
    assert (n < num_lpi);

    // Atomic load because other CPUs can update the table concurrently
    Sm *const sm { table_l[n].atomic_load() };

    if (sm) [[likely]]
        sm->up();

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handler (bool vcpu)
{
    auto const val { Gicc::ack() };
    auto const iid { val & (BIT (Gicd::ord_iid) - 1) };

    Gicc::eoi (val);

    auto const dir = [&]() { Gicc::dir (val); };

    switch (Intid::type (iid)) {
        case Type::SGI:  return handle_sgi  (Intid::to_sgi  (iid), dir);
        case Type::PPI:  return handle_ppi  (Intid::to_ppi  (iid), dir, vcpu);
        case Type::SPI:  return handle_spi  (Intid::to_spi  (iid), dir);
        case Type::EPPI: return handle_eppi (Intid::to_eppi (iid), dir);
        case Type::ESPI: return handle_espi (Intid::to_espi (iid), dir);
        case Type::LPI:  return handle_lpi  (Intid::to_lpi  (iid));
        default:         return Event::Selector::NONE;
    }
}

bool Interrupt::tmr_act_get()
{
    return Gicr::act_get (Intid::from_ppi (Timer::ppi_el1_v));
}

void Interrupt::tmr_act_set (bool a)
{
    Gicr::act_set (Intid::from_ppi (Timer::ppi_el1_v), a);
}

void Interrupt::deactivate (Sm *sm)
{
    auto const iid { sm->get_iid() };

    // An LPI does not require deactivation
    if (Intid::type (iid) == Type::LPI)
        return;

    // A guest-owned SPI/ESPI is deactivated by the guest
    if ((Intid::type (iid) == Type::SPI  && guest_s.tst (Intid::to_spi  (iid))) ||
        (Intid::type (iid) == Type::ESPI && guest_e.tst (Intid::to_espi (iid))))
        return;

    Gicc::dir (iid);
}

Status Interrupt::assign (bool attach, Sm * const sm, Dc const *dc, cpu_t cpu, uint16_t eid, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // Semaphore must be valid
    assert (sm);

    // Determine slot pointer
    auto const ptr { static_cast<Refptr<Sm> *>(sm->get_ptr()) };

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

    // Extract configuration from flags
    bool const msk { !!(cfg & BIT (0)) };
    bool const lvl { !!(cfg & BIT (1)) };
    bool const gst { !!(cfg & BIT (3)) };

    // Determine interrupt ID from interrupt semaphore
    auto const iid { sm->get_iid() };

    trace (TRACE_INTR, "INTR: Routing INTID %#06x (%c%c%c) to %#06x", iid, msk ? 'M' : 'U', lvl ? 'L' : 'E', gst ? 'G' : 'H', cpu);

    // Configure LPI
    if (iid >= Intid::BASE_LPI) [[likely]] {

        // Source device, GITS and ITT must be provided
        if (!dc || !dc->gits || !dc->itt) [[unlikely]]
            return Status::BAD_DEV;

        Gicr::conf_lpi (iid, msk);

        return dc->gits->conf_lpi (iid, cpu, dc->did, eid, msi_addr, msi_data);
    }

    // Report zero values for pin-based interrupts
    msi_addr = msi_data = 0;

    // Configure ESPI
    if (iid >= Intid::BASE_ESPI) [[likely]] {
        guest_e.cfg (Intid::to_espi (iid), gst);        // Track guest ownership
        return Gicd::conf_ext (iid, cpu, lvl, msk);
    }

    // Configure SPI
    if (iid >= Intid::BASE_SPI) [[likely]] {
        guest_s.cfg (Intid::to_spi (iid), gst);         // Track guest ownership
        return Gicd::conf_std (iid, cpu, lvl, msk);
    }

    // Should never happen
    return Status::BAD_PAR;
}

void Interrupt::send_cpu (Request req, cpu_t cpu)
{
    (Gicd::arch < 3 ? Gicd::send_cpu : Gicc::send_cpu) (req, cpu);
}

void Interrupt::send_exc (Request req)
{
    (Gicd::arch < 3 ? Gicd::send_exc : Gicc::send_exc) (req);
}
