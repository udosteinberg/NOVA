/*
 * Console: Cadence (CDNS) UART
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

class Console_cdns_mmio
{
    public:
        enum class Register
        {
            CTRL        = 0x00,     // UART Control Register
            MODE        = 0x04,     // UART Mode Register
            IDIS        = 0x0c,     // Interrupt Disable Register
            BGEN        = 0x18,     // Baud Rate Generator Register
            CSTS        = 0x2c,     // Channel Status Register
            FIFO        = 0x30,     // Transmit and Receive FIFO
            BDIV        = 0x34,     // Baud Rate Divider Register
        };
};

class Console_cdns final : private Console_cdns_mmio, private Console_uart_mmio<Console_cdns, Console_cdns_mmio::Register>
{
    friend class Console_uart;
    friend class Console_uart_mmio;

    private:
        static Console_cdns con;

        static unsigned const refclock = 100000000;

        bool tx_busy() { return (read<uint32> (Register::CSTS) & (BIT (11) | BIT (3))) != BIT (3); }
        bool tx_full() { return (read<uint32> (Register::CSTS) &  BIT (4)); }

        void tx (uint8 c)
        {
            write<uint32> (Register::FIFO, c);
        }

        void init()
        {
            write<uint32> (Register::CTRL, BIT (5) | BIT (3));
            write<uint32> (Register::MODE, BIT (5));
            write<uint32> (Register::IDIS, BIT_RANGE (13, 0));
            write<uint32> (Register::BGEN, refclock / baudrate / 5);
            write<uint32> (Register::BDIV, 4);
            write<uint32> (Register::CTRL, BIT_RANGE (4, 3) | BIT_RANGE (1, 0));
        }

        Console_cdns (Hpt::OAddr p) : Console_uart_mmio (p) {}
};
