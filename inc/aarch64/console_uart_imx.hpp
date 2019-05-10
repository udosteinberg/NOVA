/*
 * Console: i.MX UART
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
#include "util.hpp"

class Console_imx_mmio
{
    public:
        enum class Register
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
};

class Console_imx final : private Console_imx_mmio, private Console_uart_mmio<Console_imx, Console_imx_mmio::Register>
{
    friend class Console_uart;
    friend class Console_uart_mmio;

    private:
        static Console_imx con;

        static unsigned const modclock = 25000000;

        bool tx_busy() { return !(read<uint32> (Register::USR2) & BIT (3)); }
        bool tx_full() { return  (read<uint32> (Register::UTS)  & BIT (4)); }

        void tx (uint8 c)
        {
            write<uint32> (Register::UTXD, c);
        }

        void init()
        {
            write<uint32> (Register::UCR1, BIT (0));
            write<uint32> (Register::UCR2, BIT (14) | BIT (5) | BIT (2));
            write<uint32> (Register::UCR3, BIT (2));
            write<uint32> (Register::UCR4, 0);
            write<uint32> (Register::UFCR, BIT (15) | BIT (9) | BIT (7) | BIT (0));
            write<uint32> (Register::UMCR, 0);

            // HW requirement: baudrate <= modclock/16
            unsigned g = gcd (modclock, baudrate * 16);

            while (EXPECT_FALSE (read<uint32> (Register::UTS) & BIT (0)))
                pause();

            write<uint32> (Register::UBIR, baudrate * 16 / g - 1);
            write<uint32> (Register::UBMR, modclock / g - 1);
        }

        Console_imx (Hpt::OAddr p) : Console_uart_mmio (p) {}
};
