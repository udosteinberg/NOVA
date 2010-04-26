/*
 * Quantum Priority Descriptor (QPD)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

class Qpd
{          
    private:
        mword val;

    public:
        ALWAYS_INLINE
        inline explicit Qpd (mword v) : val (v) {}

        ALWAYS_INLINE
        inline unsigned long quantum() const
        {
            return val >> 12;
        }

        ALWAYS_INLINE
        inline unsigned long prio() const
        {
            return val & 0xff;
        }
};
