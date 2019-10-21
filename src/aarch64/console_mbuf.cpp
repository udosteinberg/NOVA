/*
 * Memory Buffer Console
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

#include "console_mbuf.hpp"
#include "sm.hpp"
#include "pd_kern.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_mbuf Console_mbuf::con;

Console_mbuf::Console_mbuf()
{
    if (!(regs = new Console_mbuf_mmio))
        return;

    Pd_kern::insert_user_mem (addr(), addr() + size());

    if ((sm = Sm::create (0)))
        Pd_kern::insert_user_obj (Space_obj::num - 1, Capability (sm, BIT_RANGE (1, 0)));

    enable();
}

void Console_mbuf::outc (char c)
{
    auto r = __atomic_load_n (&regs->r_ptr, __ATOMIC_RELAXED);
    auto w = __atomic_load_n (&regs->w_ptr, __ATOMIC_RELAXED) % regs->entries, n = (w + 1) % regs->entries;

    for (;;) {

        // Sanitize untrustworthy input
        r %= regs->entries;

        // Check if there is buffer space available
        if (r != n)
            break;

        // Force reader to discard oldest data. A compare_exchange failure updates r.
        if (__atomic_compare_exchange_n (&regs->r_ptr, &r, (r + 1) % regs->entries, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
            break;
    }

    // Store data
    regs->buffer[w] = c;

    // Publish new write pointer
    __atomic_store_n (&regs->w_ptr, n, __ATOMIC_SEQ_CST);

    // Signal consumer at line end
    if (c == '\n')
        sm->up();
}
