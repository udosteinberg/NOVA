/*
 * Serial Console
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
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
#include "types.h"

class Console_serial : public Console
{
    private:
        enum Port
        {
            RHR         = 0,    // Receive Holding Register     (read)
            THR         = 0,    // Transmit Holding Register    (write)
            IER         = 1,    // Interrupt Enable Register    (write)
            ISR         = 2,    // Interrupt Status Register    (read)
            FCR         = 2,    // FIFO Control Register        (write)
            LCR         = 3,    // Line Control Register        (write)
            MCR         = 4,    // Modem Control Register       (write)
            LSR         = 5,    // Line Status Register         (read)
            MSR         = 6,    // Modem Status Register        (read)
            SPR         = 7,    // Scratchpad Register          (read/write)
            DLR_LOW     = 0,
            DLR_HIGH    = 1,
        };

        enum
        {
            IER_RHR_INTR        = 1u << 0,  // Receive Holding Register Interrupt
            IER_THR_INTR        = 1u << 1,  // Transmit Holding Register Interrupt
            IER_RLS_INTR        = 1u << 2,  // Receive Line Status Interrupt
            IER_MOS_INTR        = 1u << 3,  // Modem Status Interrupt

            FCR_FIFO_ENABLE     = 1u << 0,  // FIFO Enable
            FCR_RECV_FIFO_RESET = 1u << 1,  // Receiver FIFO Reset
            FCR_TMIT_FIFO_RESET = 1u << 2,  // Transmit FIFO Reset
            FCR_DMA_MODE        = 1u << 3,  // DMA Mode Select
            FCR_RECV_TRIG_LOW   = 1u << 6,  // Receiver Trigger LSB
            FCR_RECV_TRIG_HIGH  = 1u << 7,  // Receiver Trigger MSB

            ISR_INTR_STATUS     = 1u << 0,  // Interrupt Status
            ISR_INTR_PRIO       = 7u << 1,  // Interrupt Priority

            LCR_DATA_BITS_5     = 0u << 0,
            LCR_DATA_BITS_6     = 1u << 0,
            LCR_DATA_BITS_7     = 2u << 0,
            LCR_DATA_BITS_8     = 3u << 0,
            LCR_STOP_BITS_1     = 0u << 2,
            LCR_STOP_BITS_2     = 1u << 2,
            LCR_PARITY_ENABLE   = 1u << 3,
            LCR_PARITY_EVEN     = 1u << 4,
            LCR_PARITY_FORCE    = 1u << 5,
            LCR_BREAK           = 1u << 6,
            LCR_DLAB            = 1u << 7,

            MCR_DTR             = 1u << 0,  // Data Terminal Ready
            MCR_RTS             = 1u << 1,  // Request To Send
            MCR_GPO_1           = 1u << 2,  // General Purpose Output 1
            MCR_GPO_2           = 1u << 3,  // General Purpose Output 2
            MCR_LOOP            = 1u << 4,  // Loopback Check

            LSR_RECV_READY      = 1u << 0,
            LSR_OVERRUN_ERROR   = 1u << 1,
            LSR_PARITY_ERROR    = 1u << 2,
            LSR_FRAMING_ERROR   = 1u << 3,
            LSR_BREAK_INTR      = 1u << 4,
            LSR_TMIT_HOLD_EMPTY = 1u << 5,
            LSR_TMIT_EMPTY      = 1u << 6
        };

        unsigned base;

        ALWAYS_INLINE
        inline unsigned in (Port port)
        {
            return Io::in<uint8>(base + port);
        }

        ALWAYS_INLINE
        inline void out (Port port, unsigned val)
        {
            Io::out (base + port, static_cast<uint8>(val));
        }

        void putc (int c);

    public:
        INIT
        void init();
};
