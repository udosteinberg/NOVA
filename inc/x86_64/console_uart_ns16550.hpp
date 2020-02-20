/*
 * Console: NS16550 UART
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "io.hpp"

class Console_uart_ns16550 : protected Console_uart
{
    private:
        enum class Register8 : unsigned
        {
            RBR = 0,    // r-   Receiver Buffer Register
            THR = 0,    // -w   Transmitter Holding Register
            IER = 1,    // rw   Interrupt Enable Register
            IIR = 2,    // r-   Interrupt Identification Register
            FCR = 2,    // -w   FIFO Control Register
            LCR = 3,    // rw   Line Control Register
            MCR = 4,    // rw   Modem Control Register
            LSR = 5,    // r-   Line Status Register
            MSR = 6,    // r-   Modem Status Register
            SCR = 7,    // rw   Scratch Register
            DLL = 0,    // rw   Divisor Latch Register (Lo)
            DLH = 1,    // rw   Divisor Latch Register (Hi)
        };

        uint8_t read (Register8 r) const
        {
            if (EXPECT_TRUE (mmap))
                return *reinterpret_cast<uint8_t volatile *>(mmap + std::to_underlying (r) * regs.size);
            else
                return Io::in<uint8_t>(static_cast<port_t>(regs.port + std::to_underlying (r)));
        }

        void write (Register8 r, uint8_t v) const
        {
            if (EXPECT_TRUE (mmap))
                *reinterpret_cast<uint8_t volatile *>(mmap + std::to_underlying (r) * regs.size) = v;
            else
                Io::out<uint8_t>(static_cast<port_t>(regs.port + std::to_underlying (r)), v);
        }

        bool tx_busy() const override final { return !(read (Register8::LSR) & BIT (6)); }
        bool tx_full() const override final { return !(read (Register8::LSR) & BIT (5)); }
        void tx (uint8_t c) const override final { write (Register8::THR, c); }

        bool match_dbgp (Debug::Subtype s) const override final
        {
            return s == Debug::Subtype::SERIAL_NS16550_FULL ||
                   s == Debug::Subtype::SERIAL_NS16550_SUBSET;
        }

        bool init() const override final
        {
            constexpr auto b { baudrate * 16 };

            if (clock) {
                write (Register8::LCR, BIT (7));
                write (Register8::DLL, BIT_RANGE (7, 0) & clock / b);
                write (Register8::DLH, BIT_RANGE (7, 0) & clock / b >> 8);
            }

            write (Register8::LCR, BIT_RANGE (1, 0));
            write (Register8::FCR, BIT (0));
            write (Register8::IER, 0);
            write (Register8::MCR, BIT_RANGE (1, 0));

            return !read (Register8::IER) && (!clock || read (Register8::MSR) & BIT (4));
        }

    protected:
        explicit Console_uart_ns16550 (Regs const r, unsigned c) : Console_uart (c) { setup (r); }
};
