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
#include "gicc.hpp"
#include "gicd.hpp"
#include "gicr.hpp"
#include "hazard.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"
#include "timeout.hpp"

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
        case Request::RRQ: break;
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

Event::Selector Interrupt::handle_spi (unsigned n, auto const &)
{
    assert (n < num_spi);

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

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handler (bool vcpu)
{
    auto const val { Gicc::ack() };
    auto const iid { val & iid_msk };

    Gicc::eoi (val);

    auto const dir = [&]() { Gicc::dir (val); };

    switch (Intid::type (iid)) {
        case Type::SGI:  return handle_sgi  (Intid::to_sgi  (iid), dir);
        case Type::PPI:  return handle_ppi  (Intid::to_ppi  (iid), dir, vcpu);
        case Type::SPI:  return handle_spi  (Intid::to_spi  (iid), dir);
        case Type::EPPI: return handle_eppi (Intid::to_eppi (iid), dir);
        case Type::ESPI: return handle_espi (Intid::to_espi (iid), dir);
        default:         return Event::Selector::NONE;
    }
}

bool Interrupt::tmr_act_get()
{
    return (Gicd::arch < 3 ? Gicd::act_get : Gicr::act_get) (Intid::from_ppi (Timer::ppi_el1_v));
}

void Interrupt::tmr_act_set (bool a)
{
    (Gicd::arch < 3 ? Gicd::act_set : Gicr::act_set) (Intid::from_ppi (Timer::ppi_el1_v), a);
}

void Interrupt::deactivate (Sm *)
{
    auto const iid { 0 };

    // Guest-owned interrupts are deactivated by the guest
    if ((Intid::type (iid) == Type::SPI  && guest_s.tst (Intid::to_spi  (iid))) ||
        (Intid::type (iid) == Type::ESPI && guest_e.tst (Intid::to_espi (iid))))
        return;

    Gicc::dir (iid);
}

void Interrupt::configure (iid_t iid, bool lvl, bool msk, bool gst, cpu_t cpu)
{
    switch (Intid::type (iid)) {

        case Type::PPI:
            return Gicd::arch < 3 ? Gicd::conf_std (iid, lvl, msk) : Gicr::conf_ppi (iid, lvl, msk);

        case Type::SPI:
            guest_s.cfg (Intid::to_spi (iid), gst);         // Track guest ownership for SPI
            return Gicd::conf_std (iid, lvl, msk, cpu);

        case Type::EPPI:
            return Gicr::conf_ppi (iid, lvl, msk);

        case Type::ESPI:
            guest_e.cfg (Intid::to_espi (iid), gst);        // Track guest ownership for ESPI
            return Gicd::conf_ext (iid, lvl, msk, cpu);

        default:
            break;
    }
}

Status Interrupt::assign (Sm *sm, cpu_t cpu, iid_t iid, pci_t, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // Detach
    if (!sm) [[unlikely]]
        return Status::SUCCESS;

    // Extract configuration from flags
    bool const msk { !!(cfg & BIT (0)) };
    bool const lvl { !!(cfg & BIT (1)) };
    bool const gst { !!(cfg & BIT (3)) };

    trace (TRACE_INTR, "INTR: Routing INTID %#06x (%c%c%c) to %#06x", iid, msk ? 'M' : 'U', lvl ? 'L' : 'E', gst ? 'G' : 'H', cpu);

    configure (iid, lvl, msk, gst, cpu);

    msi_addr = msi_data = 0;

    return Status::SUCCESS;
}

void Interrupt::send_cpu (Request req, cpu_t cpu)
{
    (Gicd::arch < 3 ? Gicd::send_cpu : Gicc::send_cpu) (req, cpu);
}

void Interrupt::send_exc (Request req)
{
    (Gicd::arch < 3 ? Gicd::send_exc : Gicc::send_exc) (req);
}
