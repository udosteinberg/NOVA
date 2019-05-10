/*
 * Console: i.MX UART
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
#include "util.hpp"

class Console_uart_imx final : private Console_uart
{
    private:
        static Console_uart_imx singleton;

        enum class Reg32 : unsigned
        {
            URXD        = 0x00,     // Receiver Register
            UTXD        = 0x40,     // Transmitter Register
            UCR1        = 0x80,     // Control Register 1
            UCR2        = 0x84,     // Control Register 2
            UCR3        = 0x88,     // Control Register 3
            UCR4        = 0x8c,     // Control Register 4
            UFCR        = 0x90,     // FIFO Control Register
            USR1        = 0x94,     // Status Register 1
            USR2        = 0x98,     // Status Register 2
            UESC        = 0x9c,     // Escape Character Register
            UTIM        = 0xa0,     // Escape Timer Register
            UBIR        = 0xa4,     // BRM Incremental Register
            UBMR        = 0xa8,     // BRM Modulator Register
            UBRC        = 0xac,     // Baud Rate Count Register
            ONEMS       = 0xb0,     // One Millisecond Register
            UTS         = 0xb4,     // Test Register
            UMCR        = 0xb8,     // Mode Control Register
        };

        auto read  (Reg32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)); }
        void write (Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)) = v; }

        bool tx_busy() const override final { return !(read (Reg32::USR2) & BIT (3)); }
        bool tx_full() const override final { return  (read (Reg32::UTS)  & BIT (4)); }
        void tx (uint8_t c) const override final { write (Reg32::UTXD, c); }

        bool match_dbgp (Debug::Type t, Debug::Subtype s) const override final
        {
            return t == Debug::Type::SERIAL && s == Debug::Subtype::SERIAL_IMX;
        }

        bool init() const override final
        {
            constexpr auto b { baudrate * 16 };
            constexpr auto cr1 { BIT (0) };
            constexpr auto cr2 { BIT (14) | BIT (5) | BIT (2) };
            constexpr auto cr3 { BIT (4) | BIT (2) };

            write (Reg32::UCR1, cr1);
            write (Reg32::UCR2, cr2);
            write (Reg32::UCR3, cr3);
            write (Reg32::UCR4, 0);
            write (Reg32::UFCR, BIT (15) | BIT (9) | BIT (7) | BIT (0));
            write (Reg32::UMCR, 0);

            while (EXPECT_FALSE (read (Reg32::UTS) & BIT (0)))
                pause();

            if (clock) {            // UBIR must be written before UBMR
                auto const g { gcd (clock, b) };
                write (Reg32::UBIR, b / g - 1);
                write (Reg32::UBMR, clock / g - 1);
            }

            return read (Reg32::UCR1) == cr1 && read (Reg32::UCR3) == cr3;
        }

    protected:
        explicit Console_uart_imx (Hpt::OAddr p, unsigned c) : Console_uart { c } { setup (Regs { .phys = p }); }
};
