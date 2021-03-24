/*
 * Memory-Buffer Console
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

#include "console_mbuf.hpp"
#include "pd_kern.hpp"
#include "sm.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_mbuf Console_mbuf::singleton;

Console_mbuf::Console_mbuf()
{
    if (!(regs = new Console_mbuf_mmio))
        return;

    if (!(sm = Sm::create (0)))
        return;

    Pd_kern::insert_user_mem (addr(), addr() + size());
    Pd_kern::insert_user_obj (Space_obj::num - 1, Capability (sm, static_cast<unsigned>(Capability::Perm_sm::DEFINED)));

    enable();
}

void Console_mbuf::outc (char c)
{
    // Index values: raw (r/w), current (rc/wc), next (rn/wn)
    uint32 r = regs->r_idx, rc = r % regs->size % regs->entries, rn = (rc + 1) % regs->entries;
    uint32 w = regs->w_idx, wc = w % regs->size % regs->entries, wn = (wc + 1) % regs->entries;

    // Force reader to discard oldest data if there is no buffer space available
    if (EXPECT_FALSE (wn == rc))
        regs->r_idx.compare_exchange (r, rn);

    // Publish new data
    regs->buffer[wc] = c;

    // Publish new write index
    regs->w_idx = wn;

    // Signal reader at line end
    if (c == '\n')
        sm->up();
}
