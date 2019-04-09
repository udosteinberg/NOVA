/*
 * Generic Timer
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

#include "board.hpp"
#include "interrupt.hpp"
#include "macros.hpp"
#include "stdio.hpp"
#include "timer.hpp"

uint32 Timer::freq;

void Timer::interrupt()
{
}

void Timer::init()
{
    // Determine frequency of the system counter
    freq = frequency();

    trace (TRACE_TIMR, "TIMR: PPI:%u TRG:%c FRQ:%u", Board::tmr[0].ppi, Board::tmr[0].flg & BIT_RANGE (3, 2) ? 'L' : 'E', freq);

    // Configure hypervisor timer interrupt
    Interrupt::conf_ppi (Board::tmr[0].ppi, false, Board::tmr[0].flg & BIT_RANGE (3, 2));

    // Configure virtual timer interrupt
    Interrupt::conf_ppi (Board::tmr[1].ppi, false, Board::tmr[1].flg & BIT_RANGE (3, 2));

    // EL0/EL1: pTMR disabled, pCTR disabled
    set_access (0);

    // Configure timer control
    set_control (ENABLE);
}
