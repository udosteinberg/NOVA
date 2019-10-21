/*
 * Memory Buffer Console
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "atomic.hpp"
#include "console_mbuf.hpp"
#include "sm.hpp"
#include "pd.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_mbuf Console_mbuf::con;

Console_mbuf::Console_mbuf()
{
    if (!(regs = new Console_mbuf_mmio))
        return;

    Pd::insert_mem_user (addr(), addr() + size());

    Pd::kern.Space_obj::insert (Space_obj::num - 1, Capability (sm = Sm::create (0), BIT_RANGE (1, 0)));

    enable();
}

void Console_mbuf::outc (char c)
{
    auto w = regs->w_ptr % regs->entries, n = (w + 1) % regs->entries;

    for (;;) {

        auto r = Atomic::load (regs->r_ptr) % regs->entries;

        // Check if space available
        if (r != n)
            break;

        // Force reader to discard oldest data
        if (Atomic::cmp_swap (regs->r_ptr, r, (r + 1) % regs->entries))
            break;
    }

    // Store data
    regs->buffer[w] = c;

    // Update write pointer
    Atomic::store (regs->w_ptr, n);

    // Signal at line end
    if (c == '\n')
        sm->up();
}
