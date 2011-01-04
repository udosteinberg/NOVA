/*
 * Serial Console
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.h"
#include "console.h"
#include "io.h"

class Console_serial : public Console
{
    private:
        enum Reg
        {
            RHR     = 0,    // Receive Holding Register     (read)
            THR     = 0,    // Transmit Holding Register    (write)
            IER     = 1,    // Interrupt Enable Register    (write)
            ISR     = 2,    // Interrupt Status Register    (read)
            FCR     = 2,    // FIFO Control Register        (write)
            LCR     = 3,    // Line Control Register        (write)
            MCR     = 4,    // Modem Control Register       (write)
            LSR     = 5,    // Line Status Register         (read)
            MSR     = 6,    // Modem Status Register        (read)
            SPR     = 7,    // Scratchpad Register          (read/write)
            DLR_LO  = 0,
            DLR_HI  = 1,
        };

        unsigned base;

        ALWAYS_INLINE
        inline unsigned in (Reg reg) { return Io::in<uint8>(base + reg); }

        ALWAYS_INLINE
        inline void out (Reg reg, unsigned val) { Io::out (base + reg, static_cast<uint8>(val)); }

        void putc (int c);

    public:
        INIT
        void init();
};
