/*
 * Console: NS16550 UART
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

#include "console_uart.hpp"

class Console_uart_ns16550 final : private Console_uart
{
    private:
        static Console_uart_ns16550 singleton;

        enum class Register32 : unsigned
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

        auto read  (Register32 r) const    { return *reinterpret_cast<uint32 volatile *>(mmap + std::to_underlying (r)); }
        void write (Register32 r, uint32 v) const { *reinterpret_cast<uint32 volatile *>(mmap + std::to_underlying (r)) = v; }

        bool tx_busy() const override final { return !(read (Register32::LSR) & BIT (6)); }
        bool tx_full() const override final { return !(read (Register32::LSR) & BIT (5)); }
        void tx (uint8 c) const override final { write (Register32::THR, c); }

        bool match_dbgp (Debug::Subtype s) const override final { return s == Debug::Subtype::SERIAL_NS16550_FULL; }

        void init() const override final
        {
            if (!clock)             // Keep the existing settings
                return;

            constexpr auto b { baudrate * 16 };

            write (Register32::LCR, BIT (7));
            write (Register32::DLL, BIT_RANGE (7, 0) & clock / b);
            write (Register32::DLH, BIT_RANGE (7, 0) & clock / b >> 8);
            write (Register32::LCR, BIT_RANGE (1, 0));
            write (Register32::IER, 0);
            write (Register32::FCR, BIT_RANGE (2, 0));
            write (Register32::MCR, BIT_RANGE (1, 0));
        }

    protected:
        explicit Console_uart_ns16550 (Hpt::OAddr p, unsigned c) : Console_uart (c) { setup_regs (Acpi_gas::Asid::MMIO, p); }
};
