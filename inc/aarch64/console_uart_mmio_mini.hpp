/*
 * Console: Mini UART
 *
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

#include "console_uart_mmio.hpp"

class Console_uart_mmio_mini final : private Console_uart_mmio
{
    private:
        enum class Register32 : unsigned
        {
            AUXENB      = 0x04,     // Auxiliary Enables
            MU_IO       = 0x40,     // Mini UART I/O Data
            MU_IER      = 0x44,     // Mini UART Interrupt Enable
            MU_LCR      = 0x4c,     // Mini UART Line Control
            MU_CTRL     = 0x60,     // Mini UART Extra Control
            MU_STAT     = 0x64,     // Mini UART Extra Status
            MU_BAUD     = 0x68,     // Mini UART Baudrate
        };

        inline auto read  (Register32 r)    { return *reinterpret_cast<uint32 volatile *>(mmio_base + std::to_underlying (r)); }
        inline void write (Register32 r, uint32 v) { *reinterpret_cast<uint32 volatile *>(mmio_base + std::to_underlying (r)) = v; }

        inline bool tx_busy() override final { return !(read (Register32::MU_STAT) & BIT (9)); }
        inline bool tx_full() override final { return  (read (Register32::MU_STAT) & BIT (5)); }
        inline void tx (uint8 c) override final { write (Register32::MU_IO, c); }

        void init() override final
        {
            write (Register32::AUXENB, BIT (0));
            write (Register32::MU_LCR, BIT_RANGE (1, 0));
            write (Register32::MU_IER, BIT_RANGE (2, 1));
            write (Register32::MU_BAUD, clock / baudrate / 8 - 1);
            write (Register32::MU_CTRL, BIT (1));
        }

    protected:
        Console_uart_mmio_mini (Hpt::OAddr p, unsigned c) : Console_uart_mmio (c) { mmap (p); }

    public:
        static Console_uart_mmio_mini singleton;
};
