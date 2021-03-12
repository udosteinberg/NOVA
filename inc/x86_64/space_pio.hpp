/*
 * PIO Space
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
#include "space.hpp"
#include "status.hpp"

class Space_pio : public Space
{
    private:
        struct Bitmap;

        Bitmap *bitmap { nullptr };

        NODISCARD
        bool walk (unsigned long, bool, uintptr_t *&);

    public:
        static constexpr unsigned long num = BIT64 (16);

        ~Space_pio();

        bool lookup (unsigned long);

        Status update (unsigned long, bool);
};
