/*
 * Console: Mini UART
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

class Console_mini_mmio
{
    public:
        enum class Register
        {
            AUXENB      = 0x04,     // Auxiliary Enables
            MU_IO       = 0x40,     // Mini UART I/O Data
            MU_IER      = 0x44,     // Mini UART Interrupt Enable
            MU_LCR      = 0x4c,     // Mini UART Line Control
            MU_CTRL     = 0x60,     // Mini UART Extra Control
            MU_STAT     = 0x64,     // Mini UART Extra Status
            MU_BAUD     = 0x68,     // Mini UART Baudrate
        };
};

class Console_mini final : private Console_mini_mmio, private Console_uart_mmio<Console_mini, Console_mini_mmio::Register>
{
    friend class Console_uart;
    friend class Console_uart_mmio;

    private:
        static Console_mini con;

        static unsigned const refclock = 500000000;

        bool tx_busy() { return !(read<uint32> (Register::MU_STAT) & BIT (9)); }
        bool tx_full() { return  (read<uint32> (Register::MU_STAT) & BIT (5)); }

        void tx (uint8 c)
        {
            write<uint32> (Register::MU_IO, c);
        }

        void init()
        {
            write<uint32> (Register::AUXENB, BIT (0));
            write<uint32> (Register::MU_LCR, BIT_RANGE (1, 0));
            write<uint32> (Register::MU_IER, BIT_RANGE (2, 1));
            write<uint32> (Register::MU_BAUD, refclock / baudrate / 8 - 1);
            write<uint32> (Register::MU_CTRL, BIT (1));
        }

        Console_mini (Hpt::OAddr p) : Console_uart_mmio (p) {}
};
