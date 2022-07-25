/*
 * Memory-Buffer Console
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
#include "space_hst.hpp"
#include "space_obj.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_mbuf Console_mbuf::singleton;

Console_mbuf::Console_mbuf() : regs { new Console_mbuf_mmio }
{
    if (!regs) [[unlikely]]
        return;

    Status s;
    if (!(sm = Pd::create_sm (s, &Space_obj::nova, Space_obj::Selector::NOVA_CON, 0))) [[unlikely]]
        return;

    // Grant user read/write access to memory buffer
    Space_hst::access_ctrl (addr(), size(), Paging::Permissions (Paging::U | Paging::W | Paging::R));

    enable();
}

bool Console_mbuf::outc (char c) const
{
    // Index values: raw (r/w), current (rc/wc), next (rn/wn)
    uint32_t r { regs->r_idx }, rc { r % regs->size % regs->entries }, rn { (rc + 1) % regs->entries };
    uint32_t w { regs->w_idx }, wc { w % regs->size % regs->entries }, wn { (wc + 1) % regs->entries };

    // Force reader to discard oldest data if there is no buffer space available
    if (wn == rc) [[unlikely]]
        regs->r_idx.compare_exchange (r, rn);

    // Publish new data
    regs->buffer[wc] = c;

    // Publish new write index
    regs->w_idx = wn;

    // Signal reader at line end
    if (c == '\n') [[unlikely]]
        sm->up();

    return true;
}
