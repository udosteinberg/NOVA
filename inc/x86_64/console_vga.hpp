/*
 * VGA Console
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#pragma once

#include "console.hpp"
#include "io.hpp"
#include "memory.hpp"
#include "string.hpp"

class Console_vga final : public Console
{
    private:
        enum Register
        {
            START_ADDR_HI   = 0xc,
            START_ADDR_LO   = 0xd
        };

        unsigned num, row, col;

        ALWAYS_INLINE
        static inline unsigned read (Register reg)
        {
            Io::out<uint8>(0x3d4, reg);
            return Io::in<uint8>(0x3d5);
        }

        ALWAYS_INLINE
        static inline void write (Register reg, uint8 val)
        {
            Io::out<uint8>(0x3d4, reg);
            Io::out<uint8>(0x3d5, val);
        }

        ALWAYS_INLINE
        inline void clear_all()
        {
            memset (reinterpret_cast<void *>(HV_GLOBAL_FBUF), 0, 160 * num);
        }

        ALWAYS_INLINE
        inline void clear_row (unsigned r)
        {
            memcpy (reinterpret_cast<void *>(HV_GLOBAL_FBUF), reinterpret_cast<void *>(HV_GLOBAL_FBUF + 160), 160 * r);
            memset (reinterpret_cast<void *>(HV_GLOBAL_FBUF + 160 * r), 0, 160);
        }

        void putc (int) override;

    public:
        enum Color
        {
            COLOR_BLACK         = 0x0,
            COLOR_BLUE          = 0x1,
            COLOR_GREEN         = 0x2,
            COLOR_CYAN          = 0x3,
            COLOR_RED           = 0x4,
            COLOR_MAGENTA       = 0x5,
            COLOR_YELLOW        = 0x6,
            COLOR_WHITE         = 0x7,
            COLOR_LIGHT_BLACK   = 0x8,
            COLOR_LIGHT_BLUE    = 0x9,
            COLOR_LIGHT_GREEN   = 0xa,
            COLOR_LIGHT_CYAN    = 0xb,
            COLOR_LIGHT_RED     = 0xc,
            COLOR_LIGHT_MAGENTA = 0xd,
            COLOR_LIGHT_YELLOW  = 0xe,
            COLOR_LIGHT_WHITE   = 0xf
        };

        Console_vga();

        void setup();

        ALWAYS_INLINE
        inline unsigned spinner (unsigned id) { return id < 25 - num ? 24 - id : 0; }

        ALWAYS_INLINE
        inline void put (unsigned long r, unsigned long c, Color color, int x)
        {
            *reinterpret_cast<unsigned short volatile *>(HV_GLOBAL_FBUF + r * 160 + c * 2) = static_cast<unsigned short>(color << 8 | x);
        }

        ALWAYS_INLINE
        static inline void set_page (unsigned page)
        {
            page <<= 11;    // due to odd/even addressing
            write (START_ADDR_HI, static_cast<uint8>(page >> 8));
            write (START_ADDR_LO, static_cast<uint8>(page));
        }

        static Console_vga con;
};
