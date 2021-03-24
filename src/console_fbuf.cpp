/*
 * Framebuffer Console
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "cmdline.hpp"
#include "console_fbuf.hpp"
#include "uefi.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_fbuf Console_fbuf::singleton;

Console_fbuf::Console_fbuf() : Mmio { Uefi::info.gfx.addr, Uefi::info.gfx.size, Memattr::gfx(), false }, pixel { Uefi::info.gfx.r() | Uefi::info.gfx.g() | Uefi::info.gfx.b() }, pitch { Uefi::info.gfx.pitch }, ncols { Uefi::info.gfx.res_x / font_x }, nrows { Uefi::info.gfx.res_y / font_y }
{
    // Disabled by command line
    if (Cmdline::nofbuf) [[unlikely]]
        return;

    // HW framebuffer unusable
    if (!mmio_size || mmio_size != sizeof (pixel_t) * Uefi::info.gfx.pitch * Uefi::info.gfx.res_y) [[unlikely]]
        return;

    // Determine allocation order
    auto const ord { static_cast<Buddy::order_t>(max (bit_scan_msb (mmio_size - 1) + 1, PAGE_BITS) - PAGE_BITS) };

    // Allocate SW framebuffer
    if ((fbuf = reinterpret_cast<uintptr_t>(Buddy::alloc (ord)))) [[likely]]
        enable();
}

bool Console_fbuf::outc (char c)
{
    if (c == '\r') [[unlikely]] {
        col = 0;
        return true;
    }

    if (c != '\n') [[likely]] {
        draw (col, row, c);
        if (++col < ncols) [[likely]]
            return true;
        col = 0;
    }

    if (++row == nrows) [[unlikely]] {
        scrolling = true;
        row = 0;
    }

    blit();

    return true;
}
