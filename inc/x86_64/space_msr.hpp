/*
 * MSR Space
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

#include "compiler.hpp"
#include "paging.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_msr : public Space
{
    private:
        struct Bitmap;

        Bitmap *bitmap { nullptr };

        NODISCARD
        bool walk (unsigned long, bool, uintptr_t *&, uintptr_t *&);

    public:
        ~Space_msr();

        Paging::Permissions lookup (unsigned long);

        Status update (unsigned long, Paging::Permissions);
};
