/*
 * Capability Range Descriptor (CRD)
 *
 * Copyright (C) 2008-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "types.h"

class Crd
{
    private:
        mword val;

    public:
        enum Type
        {
            MEM = 1,
            IO  = 2,
            OBJ = 3
        };

        static unsigned const whole = 0x1f;

        ALWAYS_INLINE
        inline explicit Crd() {}

        ALWAYS_INLINE
        inline explicit Crd (mword v) : val (v) {}

        ALWAYS_INLINE
        inline explicit Crd (Type t, mword b, unsigned o) : val (b << 12 | o << 7 | t) {}

        ALWAYS_INLINE
        inline Type type() const { return static_cast<Type>(val & 0x3); }

        ALWAYS_INLINE
        inline unsigned attr() const { return val >> 2 & 0x7; }

        ALWAYS_INLINE
        inline unsigned order() const { return val >> 7 & 0x1f; }

        ALWAYS_INLINE
        inline mword base() const { return val >> 12; }
};
