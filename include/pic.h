/*
 * i8259A Programmable Interrupt Controller (PIC)
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "io.h"
#include "types.h"

class Pic
{
    private:
        enum
        {
            ID_SLAVE    = 2,                // Slave is #2
            ID_MASTER   = 1u << ID_SLAVE,   // Slave on IRQ2

            PORT_SLAVE  = 0xa0,
            PORT_MASTER = 0x20,
        };

        enum Port
        {
            OCW2        = 0,            // Port Offset OCW2
            OCW3        = 0,            // Port Offset OCW3
            ICW1        = 0,            // Port Offset ICW1
            ICW2        = 1,            // Port Offset ICW2
            ICW3        = 1,            // Port Offset ICW3
            ICW4        = 1,            // Port Offset ICW4
            OCW1        = 1             // Port Offset OCW1
        };

        enum
        {
            OCW2_SELECT = 0u << 3,
            OCW3_SELECT = 1u << 3,
            ICW1_SELECT = 2u << 3,

            ICW1_ICW4   = 1u << 0,
            ICW1_SNGL   = 1u << 1,
            ICW1_ADI    = 1u << 2,
            ICW1_LTIM   = 1u << 3,
            ICW4_8086   = 1u << 0,
            ICW4_AEOI   = 1u << 1,
            ICW4_MASTER = 1u << 2,
            ICW4_BUF    = 1u << 3,
            ICW4_SFNM   = 1u << 4,
            OCW2_EOI    = 1u << 5,
            OCW2_SL     = 1u << 6,
            OCW2_ROT    = 1u << 7,
            OCW3_IRR    = 2u << 0,
            OCW3_ISR    = 3u << 0,
            OCW3_POLL   = 1u << 2,
            OCW3_SMM    = 1u << 5,
            OCW3_ESMM   = 1u << 6,
        };

        unsigned const  _id;
        unsigned const  _base;
        unsigned        _mask;

        // XXX: Spinlock
        ALWAYS_INLINE
        inline unsigned in (Port port)
        {
            return Io::in<uint8>(_base + port);
        }

        // XXX: Spinlock
        ALWAYS_INLINE
        inline void out (Port port, unsigned val)
        {
            Io::out (_base + port, static_cast<uint8>(val));
        }

        ALWAYS_INLINE
        inline unsigned irr()
        {
            out (OCW3, OCW3_SELECT | OCW3_IRR);
            return in (OCW3);
        }

        ALWAYS_INLINE
        inline unsigned isr()
        {
            out (OCW3, OCW3_SELECT | OCW3_ISR);
            return in (OCW3);
        }

        ALWAYS_INLINE
        inline unsigned imr()
        {
            return in (OCW1);
        }

    public:
        Pic (unsigned id, unsigned base, unsigned mask) :
             _id   (id),
             _base (base),
             _mask (mask) {}

        INIT
        void init (unsigned vector);

        void eoi();

        void pin_mask (unsigned pin);

        void pin_unmask (unsigned pin);

        static Pic master;
        static Pic slave;
};
