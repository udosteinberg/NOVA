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
#include "smmu.hpp"
#include "stdio.hpp"
#include "timeout.hpp"
#include "timer.hpp"

void Interrupt::rke_handler()
{
    if (Acpi::get_transition().valid())
        Cpu::hazard |= Hazard::SLEEP;
}

Event::Selector Interrupt::handle_sgi (uint32_t val, bool)
{
    auto const sgi { Intid::to_sgi (val & BIT_RANGE (9, 0)) };

    assert (sgi < NUM_SGI);

    Counter::req[sgi].inc();

    Gicc::eoi (val);

    switch (sgi) {
        case Request::RRQ: break;
        case Request::RKE: rke_handler(); break;
    }

    Gicc::dir (val);

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_ppi (uint32_t val, bool vcpu)
{
    auto const ppi { Intid::to_ppi (val & BIT_RANGE (9, 0)) };

    assert (ppi < NUM_PPI);

    Counter::loc[ppi].inc();

    Gicc::eoi (val);

    if (ppi == Timer::ppi_el1_v)        // Deactivation by guest
        return vcpu ? Event::Selector::VTIMER : Event::Selector::NONE;

    if (ppi == Timer::ppi_el2_p)        // Deactivation by host
        Timeout::check();

    Gicc::dir (val);

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_spi (uint32_t val, bool)
{
    auto const spi { Intid::to_spi (val & BIT_RANGE (9, 0)) };

    assert (spi < NUM_SPI);

    Gicc::eoi (val);

    if (Smmu::using_spi (spi)) {

        Smmu::interrupt (spi);

        Gicc::dir (val);
    }

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handler (bool vcpu)
{
    auto const val { Gicc::ack() }, i { val & BIT_RANGE (9, 0) };

    switch (Intid::type (i)) {
        case Type::SGI: return handle_sgi (val, vcpu);
        case Type::PPI: return handle_ppi (val, vcpu);
        case Type::SPI: return handle_spi (val, vcpu);
        default:        return Event::Selector::NONE;
    }
}

bool Interrupt::get_act_tmr()
{
    return (Gicd::arch < 3 ? Gicd::get_act : Gicr::get_act) (Intid::from_ppi (Timer::ppi_el1_v));
}

void Interrupt::set_act_tmr (bool a)
{
    (Gicd::arch < 3 ? Gicd::set_act : Gicr::set_act) (Intid::from_ppi (Timer::ppi_el1_v), a);
}

void Interrupt::conf_sgi (unsigned sgi, bool msk)
{
    if (Gicd::arch < 3)
        Gicd::conf (Intid::from_sgi (sgi), msk);
    else
        Gicr::conf (Intid::from_sgi (sgi), msk);
}

void Interrupt::conf_ppi (unsigned ppi, bool msk, bool lvl)
{
    if (Gicd::arch < 3)
        Gicd::conf (Intid::from_ppi (ppi), msk, lvl);
    else
        Gicr::conf (Intid::from_ppi (ppi), msk, lvl);
}

void Interrupt::conf_spi (unsigned spi, bool msk, bool lvl, cpu_t cpu)
{
    Gicd::conf (Intid::from_spi (spi), msk, lvl, cpu);
}

void Interrupt::deactivate (Sm *)
{
    auto const spi { 0 };

    // Guest-owned interrupts are deactivated by the guest
    if (guest_owned.tst (spi))
        return;

    Gicc::dir (Intid::from_spi (spi));
}

Status Interrupt::assign (Sm *, cpu_t cpu, gsi_t spi, pci_t, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // Abort attempts to reconfigure SMMU interrupts
    if (Smmu::using_spi (spi))
        return Status::ABORTED;

    // Extract configuration from flags
    bool const msk { !!(cfg & BIT (0)) };
    bool const trg { !!(cfg & BIT (1)) };
    bool const gst { !!(cfg & BIT (3)) };

    trace (TRACE_INTR, "INTR: Routing SPI %#06x (%c%c%c) to %#06x", spi, msk ? 'M' : 'U', trg ? 'L' : 'E', gst ? 'G' : 'H', cpu);

    // Configure SPI
    conf_spi (spi, msk, trg, cpu);

    // Track guest ownership
    gst ? guest_owned.set (spi) : guest_owned.clr (spi);

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
