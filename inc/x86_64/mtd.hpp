/*
 * Message Transfer Descriptor (MTD): Architecture-Independent Part
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
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

#include "memory.hpp"
#include "types.hpp"

class Mtd
{
    protected:
        uint32 mtd { 0 };

        inline explicit Mtd (uint32 v) : mtd (v) {}

    public:
        inline operator auto() const { return mtd; }
};

class Mtd_user : public Mtd
{
    public:
        static constexpr unsigned items = PAGE_SIZE / sizeof (uintptr_t);

        inline auto count() const { return mtd % items + 1; }

        inline explicit Mtd_user (uint32 v) : Mtd (v) {}
};
