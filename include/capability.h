/*
 * Capability
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

        static unsigned const perm_bits = 0x7;

    public:
        Capability() : val (0) {}

        Capability (Kobject *o, unsigned p = perm_bits) : val (reinterpret_cast<mword>(o) | p) {}

        ALWAYS_INLINE
        inline Kobject *obj() const
        {
            return reinterpret_cast<Kobject *>(val & ~perm_bits);
        }
};
