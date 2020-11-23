/*
 * Console: NS16550 UART
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

class Console_uart_mmio_ns16550 final : public Console_uart_mmio
{
    private:
        enum class Register32
        {
            THR         = 0x00,     // Transmitter Holding Register
            IER         = 0x04,     // Interrupt Enable Register
            FCR         = 0x08,     // FIFO Control Register
            LCR         = 0x0c,     // Line Control Register
            MCR         = 0x10,     // Modem Control Register
            LSR         = 0x14,     // Line Status Register
            DLL         = 0x00,     // Divisor Latch Register (Lo)
            DLH         = 0x04,     // Divisor Latch Register (Hi)
        };

        inline auto read  (Register32 r)    { return *reinterpret_cast<uint32 volatile *>(mmio_base + static_cast<unsigned>(r)); }
        inline void write (Register32 r, uint32 v) { *reinterpret_cast<uint32 volatile *>(mmio_base + static_cast<unsigned>(r)) = v; }

        inline bool tx_busy() override final { return !(read (Register32::LSR) & BIT (6)); }
        inline bool tx_full() override final { return !(read (Register32::LSR) & BIT (5)); }
        inline void tx (uint8 c) override final { write (Register32::THR, c); }

        void init() override final
        {
            if (clock) {
                auto b = baudrate * 16;
                write (Register32::LCR, BIT (7));
                write (Register32::DLL, clock / b      & BIT_RANGE (7, 0));
                write (Register32::DLH, clock / b >> 8 & BIT_RANGE (7, 0));
            }

            write (Register32::LCR, BIT_RANGE (1, 0));
            write (Register32::IER, 0);
            write (Register32::FCR, BIT_RANGE (2, 0));
            write (Register32::MCR, BIT_RANGE (1, 0));
        }

    protected:
        Console_uart_mmio_ns16550 (Hpt::OAddr p, unsigned c) : Console_uart_mmio (c) { mmap (p); }

    public:
        static Console_uart_mmio_ns16550 singleton;
};
