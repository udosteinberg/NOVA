/*
 * Kernel Object
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "macros.hpp"
#include "refcnt.hpp"

class Kobject : public Refcnt
{
    friend class Capability;

    public:
        static constexpr auto alignment { BIT (5) };

        enum class Type : uint8_t
        {
            PD,
            EC,
            SC,
            PT,
            SM,
        };

        enum class Subtype : uint8_t
        {
            NONE            = 0,

            EC_LOCAL        = 0,
            EC_GLOBAL       = 1,
            EC_VCPU_REAL    = 2,
            EC_VCPU_OFFS    = 3,

            PD              = 0,
            OBJ             = 1,
            HST             = 2,
            GST             = 3,
            DMA             = 4,
            PIO             = 5,
            MSR             = 6,
        };

    protected:
        Type    const   type;
        Subtype const   subtype;

        explicit Kobject (Type t, Subtype s = Subtype::NONE) : type { t }, subtype { s } {}
};
