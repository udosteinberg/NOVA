/*
 * Capability
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

class Kobject;

class Capability
{
    private:
        mword val;

        static mword const mask = 0x7;

    public:
        Capability() : val (0) {}

        Capability (Kobject *o, mword a) : val (a ? reinterpret_cast<mword>(o) | (a & mask) : 0) {}

        ALWAYS_INLINE
        inline Kobject *obj() const { return reinterpret_cast<Kobject *>(val & ~mask); }

        ALWAYS_INLINE
        inline unsigned prm() const { return val & mask; }
};
