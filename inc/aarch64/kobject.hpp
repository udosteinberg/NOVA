/*
 * Kernel Object
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

#include "spinlock.hpp"
#include "types.hpp"

class Kobject
{
    friend class Capability;

    protected:
        enum class Type : uint8
        {
            PD,
            EC,
            SC,
            PT,
            SM,
        };

        enum class Subtype : uint8
        {
            NONE,
            EC_LOCAL,
            EC_GLOBAL,
            EC_VCPU,
        };

        Type    const   type;
        Subtype const   subtype;
        Spinlock        lock;

    public:
        explicit Kobject (Type t, Subtype s = Subtype::NONE) : type (t), subtype (s) {}
};
