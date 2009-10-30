/*
 * VGA Console
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "cmdline.h"
#include "console_vga.h"
#include "cpu.h"
#include "memory.h"
#include "pd.h"
#include "spinlock.h"

void Console_vga::init()
{
    if (Cmdline::novga)
        return;

    Pd::kern.Space_mem::insert (VGACN_ADDR, 0,
                                Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                                 Ptab::ATTR_GLOBAL |
                                                 Ptab::ATTR_UNCACHEABLE |
                                                 Ptab::ATTR_WRITABLE),
                                0xb9000);

    set_page (1);

    initialized = true;
}

void Console_vga::putc (int c)
{
    if (EXPECT_FALSE (c == '\f')) {
        clear_all();
        _row = _col = 0;
        return;
    }

    if (EXPECT_TRUE (c != '\n')) {
        put (_row, _col, COLOR_LIGHT_WHITE, c);
        if (EXPECT_TRUE (++_col < 80))
            return;
    }

    _col = 0;

    if (EXPECT_FALSE (++_row == _num_row))
        clear_row (--_row);
}

unsigned Console_vga::init_spinner (Spinlock *lock)
{
    if (!initialized)
        return 0;

    if (lock)
        lock->lock();
    
    if (_row == --_num_row) {
        _row--;
        clear_row (_num_row);
    }

    if (lock) {
        put (_num_row, 0, Cpu::secure ? COLOR_LIGHT_GREEN : COLOR_LIGHT_RED, (Cpu::id & 0xf)["0123456789ABCDEF"]);

        for (unsigned i = 0; i < 75; i++)
            put (_num_row, i + 5, COLOR_LIGHT_BLACK, (i & 0xf)["0123456789ABCDEF"]);

        lock->unlock();
    }

    return _num_row;
}
