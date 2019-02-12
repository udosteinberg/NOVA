/*
 * Console: PrimeCell UART (PL011)
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

#pragma once

#include "console_uart_mmio.hpp"

class Console_uart_mmio_pl011 final : public Console_uart_mmio
{
    private:
        enum class Register16
        {
            DR          = 0x00,     // Data Register
            FR          = 0x18,     // Flag Register
            IBR         = 0x24,     // Integer Baud Rate Register
            FBR         = 0x28,     // Fractional Baud Rate Register
            LCR         = 0x2c,     // Line Control Register
            CR          = 0x30,     // Control Register
        };

        inline auto read  (Register16 r)    { return *reinterpret_cast<uint16 volatile *>(mmio_base + static_cast<unsigned>(r)); }
        inline void write (Register16 r, uint16 v) { *reinterpret_cast<uint16 volatile *>(mmio_base + static_cast<unsigned>(r)) = v; }

        inline bool tx_busy() override final { return read (Register16::FR) & BIT (3); }
        inline bool tx_full() override final { return read (Register16::FR) & BIT (5); }
        inline void tx (uint8 c) override final { write (Register16::DR, c); }

        void init() override final
        {
            if (!clock)             // Keep the existing settings
                return;

            auto b = baudrate * 16;
            auto i = static_cast<uint16>(clock / b);
            auto f = static_cast<uint16>(((clock << 6) + (b >> 1)) / b - (i << 6));

            write (Register16::CR,  0);
            write (Register16::IBR, i);
            write (Register16::FBR, f);
            write (Register16::LCR, BIT_RANGE (6, 4));
            write (Register16::CR,  BIT (8) | BIT (0));
        }

    protected:
        Console_uart_mmio_pl011 (Hpt::OAddr p, unsigned c) : Console_uart_mmio (c) { mmap (p); }

    public:
        static Console_uart_mmio_pl011 singleton;
};
