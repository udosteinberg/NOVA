/*
 * Console: NS16550 UART
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "console_uart_pio.hpp"
#include "macros.hpp"

class Console_ns16550_pio
{
    public:
        enum class Register
        {
            THR = 0,                // Transmit Holding Register
            IER = 1,                // Interrupt Enable Register
            FCR = 2,                // FIFO Control Register
            LCR = 3,                // Line Control Register
            MCR = 4,                // Modem Control Register
            LSR = 5,                // Line Status Register
            DLL = 0,                // Divisor Latch (LSB)
            DLM = 1,                // Divisor Latch (MSB)
        };
};

class Console_ns16550 final : private Console_ns16550_pio, private Console_uart_pio<Console_ns16550, Console_ns16550_pio::Register>
{
    friend class Console_uart;
    friend class Console_uart_pio;

    private:
        static Console_ns16550 con;

        static unsigned const freq = 115200;

        bool tx_busy() { return !(in<uint8> (Register::LSR) & BIT (6)); }
        bool tx_full() { return !(in<uint8> (Register::LSR) & BIT (5)); }

        void tx (uint8 c)
        {
            out<uint8> (Register::THR, c);
        }

        void init()
        {
            out<uint8> (Register::LCR, BIT (7));
            out<uint8> (Register::DLL, (freq / 115200) & BIT_RANGE (7, 0));
            out<uint8> (Register::DLM, (freq / 115200) >> 8);
            out<uint8> (Register::LCR, BIT_RANGE (1, 0));
            out<uint8> (Register::IER, 0);
            out<uint8> (Register::FCR, BIT_RANGE (2, 0));
            out<uint8> (Register::MCR, BIT_RANGE (1, 0));
        }

        Console_ns16550();
};
