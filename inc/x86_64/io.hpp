/*
 * I/O Ports
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "types.hpp"

class Io final
{
    public:
        template<typename T> static auto in (port_t p)
        {
            T v;
            asm volatile ("in %w1, %0" : "=a" (v) : "Nd" (p));
            return v;
        }

        template<typename T> static void out (port_t p, T v)
        {
            asm volatile ("out %0, %w1" : : "a" (v), "Nd" (p));
        }
};
