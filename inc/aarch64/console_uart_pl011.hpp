/*
 * Console: PrimeCell UART (PL011)
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

#pragma once

#include "console.hpp"
#include "memory.hpp"

class Console_pl011 final : public Console
{
    private:
        enum class Register
        {
            DR          = 0x00,     // Data Register
            RSR         = 0x04,     // Receive Status Register
            FR          = 0x18,     // Flag Register
            IBRD        = 0x24,     // Integer Baud Rate Register
            FBRD        = 0x28,     // Fractional Baud Rate Register
            LCR         = 0x2c,     // Line Control Register
            CR          = 0x30,     // Control Register
        };

        static Console_pl011 con;

        static unsigned const baudrate = 115200;

        mword const mmio_base = UART_BASE_PL;

        template <typename T>
        inline T read (Register r)
        {
            return *reinterpret_cast<T volatile *>(mmio_base + static_cast<mword>(r));
        }

        template <typename T>
        inline void write (Register r, T v)
        {
            *reinterpret_cast<T volatile *>(mmio_base + static_cast<mword>(r)) = v;
        }

        void init();
        bool fini() override;
        void outc (char) override;

    public:
        Console_pl011();
};
