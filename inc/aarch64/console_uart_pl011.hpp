/*
 * Console: PrimeCell UART (PL011)
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

class Console_uart_pl011 final : private Console_uart
{
    private:
        static Console_uart_pl011 singleton;

        enum class Reg16 : unsigned
        {
            DR          = 0x00,     // Data Register
            FR          = 0x18,     // Flag Register
            IBR         = 0x24,     // Integer Baud Rate Register
            FBR         = 0x28,     // Fractional Baud Rate Register
            LCR         = 0x2c,     // Line Control Register
            CR          = 0x30,     // Control Register
        };

        auto read  (Reg16 r) const      { return *reinterpret_cast<uint16_t volatile *>(mmap + std::to_underlying (r)); }
        void write (Reg16 r, uint16_t v) const { *reinterpret_cast<uint16_t volatile *>(mmap + std::to_underlying (r)) = v; }

        bool tx_busy() const override final { return read (Reg16::FR) & BIT (3); }
        bool tx_full() const override final { return read (Reg16::FR) & BIT (5); }
        void tx (uint8_t c) const override final { write (Reg16::DR, c); }

        bool match_dbgp (Debug::Subtype s) const override final { return s == Debug::Subtype::SERIAL_PL011; }

        bool init() const override final
        {
            if (!clock)             // Keep the existing settings
                return true;

            auto const b { baudrate * 16 };
            auto const i { static_cast<uint16_t>(clock / b) };
            auto const f { static_cast<uint16_t>(((clock << 6) + (b >> 1)) / b - (i << 6)) };

            write (Reg16::CR,  0);
            write (Reg16::IBR, i);
            write (Reg16::FBR, f);
            write (Reg16::LCR, BIT_RANGE (6, 4));
            write (Reg16::CR,  BIT (8) | BIT (0));

            return true;
        }

    protected:
        explicit Console_uart_pl011 (Hpt::OAddr p, unsigned c) : Console_uart (c) { setup_regs (Acpi_gas::Asid::MMIO, p); }
};
