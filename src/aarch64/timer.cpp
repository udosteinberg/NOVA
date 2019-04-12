/*
 * Generic Timer
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "board.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"
#include "timeout.hpp"
#include "timer.hpp"

uint32      Timer::freq         { 0 };
unsigned    Timer::ppi_el2_p    { Board::tmr[0].ppi };
unsigned    Timer::ppi_el1_v    { Board::tmr[1].ppi };
bool        Timer::lvl_el2_p    { !!(Board::tmr[0].flg & BIT_RANGE (3, 2)) };
bool        Timer::lvl_el1_v    { !!(Board::tmr[1].flg & BIT_RANGE (3, 2)) };

void Timer::interrupt()
{
    Timeout::check();
}

void Timer::init()
{
    // Determine frequency of the system counter
    freq = frequency();

    trace (TRACE_TIMR, "TIMR: EL2p:%u%c EL1v:%u%c %u Hz", ppi_el2_p, lvl_el2_p ? 'L' : 'E', ppi_el1_v, lvl_el1_v ? 'L' : 'E', freq);

    // Configure EL2p timer interrupt
    Interrupt::conf_ppi (ppi_el2_p, false, lvl_el2_p);

    // Configure EL1v timer interrupt
    Interrupt::conf_ppi (ppi_el1_v, false, lvl_el1_v);

    // EL0/EL1: pTMR disabled, pCTR disabled
    set_access (0);

    // Configure timer control
    set_control (ENABLE);
}
