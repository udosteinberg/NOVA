/*
 * Interrupt Handling
 *
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
#include "assert.hpp"
#include "counter.hpp"
#include "gicc.hpp"
#include "gicd.hpp"
#include "gicr.hpp"
#include "hazard.hpp"
#include "interrupt.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "timer.hpp"

Interrupt Interrupt::int_table[NUM_SPI];

unsigned Interrupt::num_pin()
{
    return Intid::to_spi (Gicd::intid);
}

void Interrupt::rke_handler()
{
    if (Acpi::get_transition().state())
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
        Stc::interrupt();

    Gicc::dir (val);

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_spi (uint32_t val, bool)
{
    auto const spi { Intid::to_spi (val & BIT_RANGE (9, 0)) };

    assert (spi < NUM_SPI);

    Gicc::eoi (val);

    if (true) {

        Smmu::interrupt (spi);

        Gicc::dir (val);
    }

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handler (bool vcpu)
{
    auto const val { Gicc::ack() }, i { val & BIT_RANGE (9, 0) };

    if (i < BASE_PPI)
        return handle_sgi (val, vcpu);

    if (i < BASE_SPI)
        return handle_ppi (val, vcpu);

    if (i < BASE_RSV)
        return handle_spi (val, vcpu);

    return Event::Selector::NONE;
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

void Interrupt::conf_spi (unsigned spi, bool msk, bool lvl, unsigned cpu)
{
    Gicd::conf (Intid::from_spi (spi), msk, lvl, cpu);
}

void Interrupt::configure (unsigned spi, Config cfg, uint32_t &msi_addr, uint16_t &msi_data)
{
    trace (TRACE_INTR, "INTR: SPI:%#06x %c%c%c routed to CPU:%u", spi, cfg.msk() ? 'M' : 'U', cfg.trg() ? 'L' : 'E', cfg.gst() ? 'G' : 'H', cfg.cpu());

    int_table[spi].config = cfg;

    conf_spi (spi, cfg.msk(), cfg.trg(), cfg.cpu());

    msi_addr = msi_data = 0;
}

void Interrupt::deactivate (unsigned spi)
{
    Gicc::dir (Intid::from_spi (spi));
}

void Interrupt::send_cpu (Request req, unsigned cpu)
{
    (Gicd::arch < 3 ? Gicd::send_cpu : Gicc::send_cpu) (req, cpu);
}

void Interrupt::send_exc (Request req)
{
    (Gicd::arch < 3 ? Gicd::send_exc : Gicc::send_exc) (req);
}

void Interrupt::init()
{
    assert (num_pin() <= sizeof (int_table) / sizeof (*int_table));

    for (unsigned spi { 0 }; spi < num_pin(); spi++) {

        // Don't touch SMMU interrupts
        if (Smmu::using_spi (spi))
            continue;

        Config cfg { int_table[spi].config };

        // Interrupt is not assigned to this CPU
        if (Cpu::id != cfg.cpu())
            continue;

        // Configure interrupt
        conf_spi (spi, cfg.msk(), cfg.trg(), cfg.cpu());
    }
}
