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

#include "console_uart_mmio.hpp"

class Console_pl011_mmio
{
    public:
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
};

class Console_pl011 final : private Console_pl011_mmio, private Console_uart_mmio<Console_pl011, Console_pl011_mmio::Register>
{
    friend class Console_uart;
    friend class Console_uart_mmio;

    private:
        static Console_pl011 con;

        bool tx_busy() { return read<uint16> (Register::FR) & BIT (3); }
        bool tx_full() { return read<uint16> (Register::FR) & BIT (5); }

        void tx (uint8 c)
        {
            write<uint8> (Register::DR, c);
        }

        void init()
        {
            write<uint16> (Register::CR,  0);
            write<uint16> (Register::LCR, BIT_RANGE (6, 4));
            write<uint16> (Register::CR,  BIT (8) | BIT (0));
        }

        Console_pl011 (Hpt::OAddr p) : Console_uart_mmio (p) {}
};
