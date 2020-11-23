/*
 * Console: NS16550 UART
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
            THR = 0,                // Transmitter Holding Register
            IER = 1,                // Interrupt Enable Register
            FCR = 2,                // FIFO Control Register
            LCR = 3,                // Line Control Register
            MCR = 4,                // Modem Control Register
            LSR = 5,                // Line Status Register
            DLL = 0,                // Divisor Latch Register (Lo)
            DLH = 1,                // Divisor Latch Register (Hi)
        };

        auto read  (Register32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r) * 4); }
        void write (Register32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r) * 4) = v; }

        bool tx_busy() const override final { return !(read (Register32::LSR) & BIT (6)); }
        bool tx_full() const override final { return !(read (Register32::LSR) & BIT (5)); }
        void tx (uint8_t c) const override final { write (Register32::THR, c); }

        bool match_dbgp (Debug::Subtype s) const override final
        {
            return s == Debug::Subtype::SERIAL_NS16550_FULL ||
                   s == Debug::Subtype::SERIAL_NS16550_SUBSET;
        }

        void init() const override final
        {
            constexpr auto b { baudrate * 16 };

            if (clock) {
                write (Register32::LCR, BIT (7));
                write (Register32::DLL, BIT_RANGE (7, 0) & clock / b);
                write (Register32::DLH, BIT_RANGE (7, 0) & clock / b >> 8);
            }

            write (Register32::LCR, BIT_RANGE (1, 0));
            write (Register32::IER, 0);
            write (Register32::FCR, BIT (0));
            write (Register32::MCR, BIT_RANGE (1, 0));
        }

    protected:
        explicit Console_uart_ns16550 (Hpt::OAddr p, unsigned c) : Console_uart (c) { setup_regs (Acpi_gas::Asid::MMIO, p); }
};
