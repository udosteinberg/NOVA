/*
 * Object Capability
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "std.hpp"

class Capability final
{
    private:
        uintptr_t val { 0 };

        ALWAYS_INLINE
        inline bool validate (Kobject::Type t, unsigned p) const
        {
            return obj() && obj()->type == t && (prm() & p) == p;
        }

        ALWAYS_INLINE
        inline bool validate (Kobject::Type t, Kobject::Subtype s, unsigned p) const
        {
            return obj() && obj()->type == t && obj()->subtype == s && (prm() & p) == p;
        }

    public:
        // Mask of low bits that can store permissions
        static constexpr uintptr_t pmask { Kobject::alignment - 1 };

        // Space Capability Permissions
        enum class Perm_sp : unsigned
        {
            GRANT       = BIT (0),
            TAKE        = BIT (1),
            ASSIGN      = BIT (2),
            DEFINED_OBJ = TAKE | GRANT,
            DEFINED_HST = TAKE | GRANT,
            DEFINED_GST = ASSIGN | GRANT,
            DEFINED_DMA = ASSIGN | GRANT,
            DEFINED_PIO = ASSIGN | TAKE | GRANT,
            DEFINED_MSR = ASSIGN | TAKE | GRANT,
        };

        // PD Object Capability Permissions
        enum class Perm_pd : unsigned
        {
            CTRL        = BIT (0),
            PD          = BIT (1),
            EC_PT_SM    = BIT (2),
            SC          = BIT (3),
            ASSIGN      = BIT (4),
            DEFINED     = ASSIGN | SC | EC_PT_SM | PD | CTRL,
        };

        // EC Object Capability Permissions
        enum class Perm_ec : unsigned
        {
            CTRL        = BIT (0),
            BIND_PT     = BIT (2),
            BIND_SC     = BIT (3),
            DEFINED     = BIND_SC | BIND_PT | CTRL,
        };

        // SC Object Capability Permissions
        enum class Perm_sc : unsigned
        {
            CTRL        = BIT (0),
            DEFINED     = CTRL,
        };

        // PT Object Capability Permissions
        enum class Perm_pt : unsigned
        {
            CTRL        = BIT (0),
            CALL        = BIT (1),
            EVENT       = BIT (2),
            DEFINED     = EVENT | CALL | CTRL,
        };

        // SM Object Capability Permissions
        enum class Perm_sm : unsigned
        {
            CTRL_UP     = BIT (0),
            CTRL_DN     = BIT (1),
            ASSIGN      = BIT (4),
            DEFINED     = CTRL_DN | CTRL_UP,
            DEFINED_INT = CTRL_DN | ASSIGN,
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
        inline auto validate (Perm_sp p, Kobject::Subtype s) const { return validate (Kobject::Type::PD, s, std::to_underlying (p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_pd p) const { return validate (Kobject::Type::PD, std::to_underlying (p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_ec p) const { return validate (Kobject::Type::EC, std::to_underlying (p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_sc p) const { return validate (Kobject::Type::SC, std::to_underlying (p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_pt p) const { return validate (Kobject::Type::PT, std::to_underlying (p)); }

        ALWAYS_INLINE
        inline auto validate (Perm_sm p) const { return validate (Kobject::Type::SM, std::to_underlying (p)); }

        ALWAYS_INLINE
        static inline bool validate_take_grant (Capability const &cst, Capability const &cdt, Kobject::Subtype &st, Kobject::Subtype &dt)
        {
            // Check capability permissions: cst requires TAKE, cdt requires GRANT
            if (!(cst.prm() & std::to_underlying (Perm_sp::TAKE)) || !(cdt.prm() & std::to_underlying (Perm_sp::GRANT)))
                return false;

            // Non-zero permissions imply both capabilities are not null capabilities
            auto const s { cst.obj() };
            auto const d { cdt.obj() };

            st = s->subtype;
            dt = d->subtype;

            // Check capability types: both capabilities must be PD subtypes
            return s->type == Kobject::Type::PD && d->type == Kobject::Type::PD;
        }
};
