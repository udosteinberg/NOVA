/*
 * 8259A Programmable Interrupt Controller (PIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "io.hpp"
#include "macros.hpp"
#include "types.hpp"
#include "vectors.hpp"

class Pic
{
    public:
        ALWAYS_INLINE
        static inline void init()
        {
            Io::out<uint8>(0x20, BIT (4) | BIT (0));    // ICW1: ICW4 needed
            Io::out<uint8>(0x21, VEC_GSI);              // ICW2: Interrupt vector base
            Io::out<uint8>(0x21, BIT (2));              // ICW3: Slave on pin 2
            Io::out<uint8>(0x21, BIT (0));              // ICW4: 8086 mode
            Io::out<uint8>(0x21, BIT_RANGE (7, 0));     // OCW1: Mask everything
        }
};
