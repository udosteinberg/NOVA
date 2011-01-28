/*
 * Keyboard
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

class Keyb
{
    private:
        enum
        {
            STS_OUTB = 1U << 0,
            STS_AUXB = 1U << 5,
        };

        ALWAYS_INLINE
        static inline unsigned output() { return Io::in<uint8>(0x60); }

        ALWAYS_INLINE
        static inline unsigned status() { return Io::in<uint8>(0x64); }

    public:
        static unsigned const irq = 1;
        static unsigned gsi;

        INIT
        static void init();

        static void interrupt();
};
