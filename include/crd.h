/*
 * Capability Range Descriptor (CRD)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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
            OBJ = 3,
        };

        static unsigned const whole = 0x1f;

        ALWAYS_INLINE
        inline explicit Crd() : val (0) {}

        ALWAYS_INLINE
        inline explicit Crd (mword v) : val (v) {}

        ALWAYS_INLINE
        inline explicit Crd (Type t, mword b, mword o, mword a) : val (b << 12 | o << 7 | a << 2 | t) {}

        ALWAYS_INLINE
        inline Type type() const { return static_cast<Type>(val & 0x3); }

        ALWAYS_INLINE
        inline unsigned attr() const { return val >> 2 & 0x1f; }

        ALWAYS_INLINE
        inline unsigned order() const { return val >> 7 & 0x1f; }

        ALWAYS_INLINE
        inline mword base() const { return val >> 12; }
};

class Xfer : public Crd
{
    private:
        mword val;

    public:
        ALWAYS_INLINE
        inline explicit Xfer (Crd c, mword v) : Crd (c), val (v) {}

        ALWAYS_INLINE
        inline mword flags() const { return val & 0xfff; }

        ALWAYS_INLINE
        inline mword hotspot() const { return val >> 12; }
};
