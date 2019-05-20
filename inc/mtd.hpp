/*
 * Message Transfer Descriptor (MTD): Architecture-Independent Part
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
        using mtd_t = uint32_t;

        mtd_t const mtd;

        explicit Mtd (mtd_t v) : mtd { v } {}

    public:
        operator mtd_t() const { return mtd; }
};

class Mtd_user final : public Mtd
{
    public:
        static constexpr auto items { PAGE_SIZE (0) / sizeof (uintptr_t) };

        auto count() const { return mtd % items + 1; }

        explicit Mtd_user (mtd_t v) : Mtd { v } {}
};
