/*
 * Serial Console
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "console.hpp"
#include "io.hpp"

class Console_serial final : public Console
{
    private:
        enum Register
        {
            THR = 0,                    // Transmit Holding Register
            IER = 1,                    // Interrupt Enable Register
            FCR = 2,                    // FIFO Control Register
            LCR = 3,                    // Line Control Register
            MCR = 4,                    // Modem Control Register
            LSR = 5,                    // Line Status Register
            DLL = 0,                    // Divisor Latch (LSB)
            DLM = 1,                    // Divisor Latch (MSB)
        };

        static unsigned const freq = 115200;

        unsigned base;

        ALWAYS_INLINE
        inline unsigned in (Register r) { return Io::in<uint8>(base + r); }

        ALWAYS_INLINE
        inline void out (Register r, unsigned v) { Io::out (base + r, static_cast<uint8>(v)); }

        void outc (char) override final;

    public:
        Console_serial();

        static Console_serial con;
};
