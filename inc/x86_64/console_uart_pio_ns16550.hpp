/*
 * Console: NS16550 UART
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

class Console_uart_pio_ns16550 : private Console_uart_pio
{
    private:
        enum class Register8 : unsigned
        {
            THR = 0,                // Transmitter Holding Register
            IER = 1,                // Interrupt Enable Register
            FCR = 2,                // FIFO Control Register
            LCR = 3,                // Line Control Register
            MCR = 4,                // Modem Control Register
            LSR = 5,                // Line Status Register
            DLL = 0,                // Divisor Latch Register (Lo)
            DLH = 1,                // Divisor Latch Register (Hi)
        };

        auto read  (Register8 r)   { return Io::in <uint8>(port_base + std::to_underlying (r)); }
        void write (Register8 r, uint8 v) { Io::out<uint8>(port_base + std::to_underlying (r), v); }

        bool tx_busy() override final { return !(read (Register8::LSR) & BIT (6)); }
        bool tx_full() override final { return !(read (Register8::LSR) & BIT (5)); }
        void tx (uint8 c) override final { write (Register8::THR, c); }

        void init() override final
        {
            constexpr auto b { baudrate * 16 };

            write (Register8::LCR, BIT (7));
            write (Register8::DLL, BIT_RANGE (7, 0) & clock / b);
            write (Register8::DLH, BIT_RANGE (7, 0) & clock / b >> 8);
            write (Register8::LCR, BIT_RANGE (1, 0));
            write (Register8::IER, 0);
            write (Register8::FCR, BIT_RANGE (2, 0));
            write (Register8::MCR, BIT_RANGE (1, 0));
        }

    protected:
        explicit Console_uart_pio_ns16550 (uint16 p, unsigned c = 0) : Console_uart_pio (c) { pmap (p); }
};
