/*
 * Console: i.MX UART
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
#include "util.hpp"

class Console_uart_mmio_imx final : public Console_uart_mmio
{
    private:
        enum class Register32 : unsigned
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

        inline auto read  (Register32 r)    { return *reinterpret_cast<uint32 volatile *>(mmio_base + std::to_underlying (r)); }
        inline void write (Register32 r, uint32 v) { *reinterpret_cast<uint32 volatile *>(mmio_base + std::to_underlying (r)) = v; }

        inline bool tx_busy() override final { return !(read (Register32::USR2) & BIT (3)); }
        inline bool tx_full() override final { return  (read (Register32::UTS)  & BIT (4)); }
        inline void tx (uint8 c) override final { write (Register32::UTXD, c); }

        void init() override final
        {
            if (!clock)             // Keep the existing settings
                return;

            write (Register32::UCR1, BIT (0));
            write (Register32::UCR2, BIT (14) | BIT (5) | BIT (2));
            write (Register32::UCR3, BIT (4) | BIT (2));
            write (Register32::UCR4, 0);
            write (Register32::UFCR, BIT (15) | BIT (9) | BIT (7) | BIT (0));
            write (Register32::UMCR, 0);

            // HW requirement: baudrate <= modclock/16
            auto g { gcd (clock, baudrate * 16) };

            while (EXPECT_FALSE (read (Register32::UTS) & BIT (0)))
                pause();

            write (Register32::UBIR, baudrate * 16 / g - 1);
            write (Register32::UBMR, clock / g - 1);
        }

    protected:
        Console_uart_mmio_imx (Hpt::OAddr p, unsigned c) : Console_uart_mmio (c) { mmap (p); }

    public:
        static Console_uart_mmio_imx singleton;
};
