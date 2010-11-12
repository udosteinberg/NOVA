/*
 * VGA Console
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "pd.h"

void Console_vga::init()
{
    if (Cmdline::novga)
        return;

    Pd::kern.Space_mem::insert (VGACN_ADDR, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, 0xb9000);

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
    char const *dig = "0123456789ABCDEF";

    if (!initialized)
        return 0;

    if (lock)
        lock->lock();

    if (_row == --_num_row) {
        _row--;
        clear_row (_num_row);
    }

    if (lock) {
        put (_num_row, 0, COLOR_LIGHT_RED, (Cpu::id / 10)[dig]);
        put (_num_row, 1, COLOR_LIGHT_RED, (Cpu::id % 10)[dig]);

        for (unsigned i = SPN_GSI; i < 80; i++)
            put (_num_row, i, COLOR_LIGHT_BLACK, ((i - SPN_GSI) & 0xf)[dig]);

        lock->unlock();
    }

    return _num_row;
}
