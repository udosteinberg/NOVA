/*
 * Capability
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "kobject.hpp"
#include "macros.hpp"

class Capability
{
    private:
        uintptr_t val { 0 };

        static uintptr_t const pmask = BIT_RANGE (4, 0);

    public:
        explicit Capability (uintptr_t v) : val (v) {}

        Capability (Kobject *o, unsigned p) : val (p ? reinterpret_cast<uintptr_t>(o) | (p & pmask) : 0) {}

        ALWAYS_INLINE
        inline Kobject *obj() const { return reinterpret_cast<Kobject *>(val & ~pmask); }

        ALWAYS_INLINE
        inline unsigned prm() const { return val & pmask; }

        ALWAYS_INLINE
        inline bool validate (Kobject::Type t, unsigned p) const
        {
            return obj() && obj()->type == t && prm() & BIT (p);
        }
};
