/*
 * MSR Space
 *
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

#include "space.hpp"

class Space_msr final : public Space
{
    public:
        [[nodiscard]] auto delegate (Space_msr const *, unsigned long, unsigned long, unsigned, unsigned) { return Status::BAD_FTR; };

        [[nodiscard]] static Space_msr *create (Status &s, Slab_cache &, Pd *)
        {
            s = Status::BAD_FTR;

            return nullptr;
        }

        void destroy() {}
};
