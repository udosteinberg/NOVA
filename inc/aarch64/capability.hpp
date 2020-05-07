/*
 * Object Capability
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

#include "compiler.hpp"
#include "kobject.hpp"
#include "syscall.hpp"

class Capability
{
    private:
        uintptr_t val { 0 };

        // Mask of low bits that can store permissions
        static constexpr uintptr_t pmask = Kobject::alignment - 1;

        ALWAYS_INLINE
        inline bool validate (Kobject::Type t, unsigned p) const
        {
            return obj() && obj()->type == t && (prm() & p) == p;
        }

    public:
        // PD Object Capability Permissions
        enum class Perm_pd
        {
            CTRL        = BIT (0),
            PD          = BIT (1),
            EC_PT_SM    = BIT (2),
            SC          = BIT (3),
            ASSIGN      = BIT (4),
            DEFINED     = ASSIGN | SC | EC_PT_SM | PD | CTRL,
        };

        // EC Object Capability Permissions
        enum class Perm_ec
        {
            CTRL        = BIT (0),
            BIND_PT     = BIT (2),
            BIND_SC     = BIT (3),
            DEFINED     = BIND_SC | BIND_PT | CTRL,
        };

        // SC Object Capability Permissions
        enum class Perm_sc
        {
            CTRL        = BIT (0),
            DEFINED     = CTRL,
        };

        // PT Object Capability Permissions
        enum class Perm_pt
        {
            CTRL        = BIT (0),
            CALL        = BIT (1),
            EVENT       = BIT (2),
            DEFINED     = EVENT | CALL | CTRL,
        };

        // SM Object Capability Permissions
        enum class Perm_sm
        {
            CTRL_UP     = BIT (0),
            CTRL_DN     = BIT (1),
            ASSIGN      = BIT (4),
            DEFINED     = ASSIGN | CTRL_DN | CTRL_UP,
        };

        // Raw Capability Constructor
        inline explicit Capability (uintptr_t v) : val (v) {}

        // Null Capability Constructor
        inline Capability() : val (0) {}

        // Object Capability Constructor
        inline Capability (Kobject *o, unsigned p) : val (p ? reinterpret_cast<uintptr_t>(o) | (p & pmask) : 0) {}

        ALWAYS_INLINE
        inline Kobject *obj() const { return reinterpret_cast<Kobject *>(val & ~pmask); }

        ALWAYS_INLINE
        inline unsigned prm() const { return val & pmask; }

        ALWAYS_INLINE
        inline auto validate (Perm_pd p) const { return validate (Kobject::Type::PD, static_cast<unsigned>(p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_ec p) const { return validate (Kobject::Type::EC, static_cast<unsigned>(p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_sc p) const { return validate (Kobject::Type::SC, static_cast<unsigned>(p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_pt p) const { return validate (Kobject::Type::PT, static_cast<unsigned>(p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_sm p) const { return validate (Kobject::Type::SM, static_cast<unsigned>(p)); }
};
