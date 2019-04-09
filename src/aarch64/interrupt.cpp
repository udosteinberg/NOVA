/*
 * Interrupt Handling
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "assert.hpp"
#include "counter.hpp"
#include "gicc.hpp"
#include "gicd.hpp"
#include "gicr.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"

unsigned Interrupt::count()
{
    return Gicd::ints - BASE_SPI;
}

Event::Selector Interrupt::handle_sgi (uint32 val, bool)
{
    auto sgi = Intid::to_sgi (val & 0x3ff);

    assert (sgi < NUM_SGI);

    Counter::req[sgi].inc();

    Gicc::eoi (val);

    switch (sgi) {
        case Request::RRQ: break;
        case Request::RKE: break;
    }

    Gicc::dir (val);

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_ppi (uint32 val, bool)
{
    auto ppi = Intid::to_ppi (val & 0x3ff);

    assert (ppi < NUM_PPI);

    Counter::loc[ppi].inc();

    Gicc::eoi (val);

    Event::Selector evt = Event::Selector::NONE;

    if (evt == Event::Selector::NONE)
        Gicc::dir (val);

    return evt;
}

Event::Selector Interrupt::handle_spi (uint32 val, bool)
{
    auto spi = Intid::to_spi (val & 0x3ff);

    assert (spi < NUM_SPI);

    Gicc::eoi (val);

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handler (bool vcpu)
{
    uint32 val = Gicc::ack(), i = val & 0x3ff;

    if (i < BASE_PPI)
        return handle_sgi (val, vcpu);

    if (i < BASE_SPI)
        return handle_ppi (val, vcpu);

    if (i < BASE_RSV)
        return handle_spi (val, vcpu);

    return Event::Selector::NONE;
}

void Interrupt::conf_sgi (unsigned sgi, bool msk)
{
    if (Gicd::arch < 3)
        Gicd::conf (sgi + BASE_SGI, false);
    else
        Gicr::conf (sgi + BASE_SGI, false);

    (Gicd::arch < 3 ? Gicd::mask : Gicr::mask) (sgi + BASE_SGI, msk);
}

void Interrupt::conf_ppi (unsigned ppi, bool msk, bool lvl)
{
    if (Gicd::arch < 3)
        Gicd::conf (ppi + BASE_PPI, lvl);
    else
        Gicr::conf (ppi + BASE_PPI, lvl);

    (Gicd::arch < 3 ? Gicd::mask : Gicr::mask) (ppi + BASE_PPI, msk);
}

void Interrupt::conf_spi (unsigned spi, unsigned cpu, bool msk, bool lvl)
{
    Gicd::conf (spi + BASE_SPI, lvl, cpu);
    Gicd::mask (spi + BASE_SPI, msk);
}

void Interrupt::send_cpu (unsigned cpu, Request req)
{
    (Gicd::arch < 3 ? Gicd::send_sgi : Gicc::send_sgi) (cpu, req);
}
