/*
 * Console: Cadence (CDNS) UART
 *
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

#include "console_uart.hpp"

class Console_uart_cdns final : private Console_uart
{
    private:
        static Console_uart_cdns singleton;

        enum class Reg32 : unsigned
        {
            CTRL        = 0x00,     // UART Control Register
            MODE        = 0x04,     // UART Mode Register
            IDIS        = 0x0c,     // Interrupt Disable Register
            BGEN        = 0x18,     // Baud Rate Generator Register
            CSTS        = 0x2c,     // Channel Status Register
            FIFO        = 0x30,     // Transmit and Receive FIFO
            BDIV        = 0x34,     // Baud Rate Divider Register
        };

        auto read  (Reg32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)); }
        void write (Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)) = v; }

        bool tx_busy() const override final { return (read (Reg32::CSTS) & (BIT (11) | BIT (3))) != BIT (3); }
        bool tx_full() const override final { return (read (Reg32::CSTS) &  BIT (4)); }
        void tx (uint8_t c) const override final { write (Reg32::FIFO, c); }

        bool init() const override final
        {
            write (Reg32::CTRL, BIT (5) | BIT (3));
            write (Reg32::MODE, BIT (5));
            write (Reg32::IDIS, BIT_RANGE (13, 0));
            write (Reg32::BGEN, clock / baudrate / 5);
            write (Reg32::BDIV, 4);
            write (Reg32::CTRL, BIT_RANGE (4, 3) | BIT_RANGE (1, 0));

            return true;
        }

    protected:
        explicit Console_uart_cdns (Hpt::OAddr p, unsigned c) : Console_uart { c } { setup (Regs { .phys = p }); }
};
