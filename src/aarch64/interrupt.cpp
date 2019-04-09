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

Event::Selector Interrupt::handle_sgi (uint32 val, bool)
{
    unsigned sgi = (val & 0x3ff) - SGI_BASE;

    assert (sgi < SGI_NUM);

    Counter::count_sgi (sgi);

    switch (sgi) {
        case Sgi::RRQ: break;
        case Sgi::RKE: break;
    }

    Gicc::eoi (val);
    Gicc::dir (val);

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handle_ppi (uint32 val, bool)
{
    unsigned ppi = (val & 0x3ff) - PPI_BASE;

    assert (ppi < PPI_NUM);

    Counter::count_ppi (ppi);

    Event::Selector evt = Event::Selector::NONE;

    Gicc::eoi (val);

    if (evt == Event::Selector::NONE)
        Gicc::dir (val);

    return evt;
}

Event::Selector Interrupt::handle_spi (uint32 val, bool)
{
    unsigned spi = (val & 0x3ff) - SPI_BASE;

    assert (spi < SPI_NUM);

    switch (spi) {

        default:
            Gicc::eoi (val);
            break;
    }

    return Event::Selector::NONE;
}

Event::Selector Interrupt::handler (bool vcpu)
{
    uint32 val = Gicc::ack(), i = val & 0x3ff;

    if (i < PPI_BASE)
        return handle_sgi (val, vcpu);

    if (i < SPI_BASE)
        return handle_ppi (val, vcpu);

    if (i < RSV_BASE)
        return handle_spi (val, vcpu);

    return Event::Selector::NONE;
}

void Interrupt::conf_sgi (unsigned sgi, bool msk)
{
    trace (TRACE_INTR, "INTR: %s: %u %c", __func__, sgi, msk ? 'M' : 'U');

    if (Gicd::arch < 3)
        Gicd::conf (sgi + SGI_BASE);
    else
        Gicr::conf (sgi + SGI_BASE);

    (Gicd::arch < 3 ? Gicd::mask : Gicr::mask) (sgi + SGI_BASE, msk);
}

void Interrupt::conf_ppi (unsigned ppi, bool msk, bool trg)
{
    trace (TRACE_INTR, "INTR: %s: %u %c%c", __func__, ppi, msk ? 'M' : 'U', trg ? 'E' : 'L');

    if (Gicd::arch < 3)
        Gicd::conf (ppi + PPI_BASE, trg);
    else
        Gicr::conf (ppi + PPI_BASE, trg);

    (Gicd::arch < 3 ? Gicd::mask : Gicr::mask) (ppi + PPI_BASE, msk);
}

void Interrupt::conf_spi (unsigned spi, unsigned cpu, bool msk, bool trg, bool gst)
{
    trace (TRACE_INTR, "INTR: %s: %u cpu=%u %c%c%c", __func__, spi, cpu, msk ? 'M' : 'U', trg ? 'E' : 'L', gst ? 'G' : 'H');

    Gicd::conf (spi + SPI_BASE, trg, cpu);
    Gicd::mask (spi + SPI_BASE, msk);
}

void Interrupt::send_sgi (unsigned cpu, Sgi sgi)
{
    (Gicd::arch < 3 ? Gicd::send_sgi : Gicc::send_sgi) (cpu, sgi);
}
