/*
 * Console: Mini UART
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

class Console_uart_mini final : private Console_uart
{
    private:
        static Console_uart_mini singleton;

        enum class Reg32 : unsigned
        {
            AUXENB      = 0x04,     // Auxiliary Enables
            MU_IO       = 0x40,     // Mini UART I/O Data
            MU_IER      = 0x44,     // Mini UART Interrupt Enable
            MU_LCR      = 0x4c,     // Mini UART Line Control
            MU_CTRL     = 0x60,     // Mini UART Extra Control
            MU_STAT     = 0x64,     // Mini UART Extra Status
            MU_BAUD     = 0x68,     // Mini UART Baudrate
        };

        auto read  (Reg32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)); }
        void write (Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)) = v; }

        bool tx_busy() const override final { return !(read (Reg32::MU_STAT) & BIT (9)); }
        bool tx_full() const override final { return  (read (Reg32::MU_STAT) & BIT (5)); }
        void tx (uint8_t c) const override final { write (Reg32::MU_IO, c); }

        bool match_dbgp (Debug::Type t, Debug::Subtype s) const override final
        {
            return t == Debug::Type::SERIAL && s == Debug::Subtype::SERIAL_BCM2835;
        }

        bool init() const override final
        {
            write (Reg32::AUXENB, BIT (0));
            write (Reg32::MU_LCR, BIT_RANGE (1, 0));
            write (Reg32::MU_IER, BIT_RANGE (2, 1));
            write (Reg32::MU_BAUD, clock / baudrate / 8 - 1);
            write (Reg32::MU_CTRL, BIT (1));

            return true;
        }

    protected:
        explicit Console_uart_mini (Hpt::OAddr p, unsigned c) : Console_uart { c } { setup (Regs { .phys = p }); }
};
