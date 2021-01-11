/*
 * MSR Space
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

#include "bitmap_msr.hpp"
#include "kmem.hpp"
#include "msr.hpp"
#include "paging.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_msr : public Space
{
    friend class Pd;

    private:
        Bitmap_msr *const bmp;

        static Space_msr nova;

        Space_msr();

        inline Space_msr (Bitmap_msr *b) : bmp (b) {}

        inline ~Space_msr() { delete bmp; }

        [[nodiscard]] Paging::Permissions lookup (unsigned long) const;

        void update (unsigned long, Paging::Permissions);

    public:
        [[nodiscard]] Status delegate (Space_msr const *, unsigned long, unsigned long, unsigned, unsigned);

        [[nodiscard]] inline auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        static void user_access (Msr::Register r, Paging::Permissions p) { nova.update (std::to_underlying (r), p); }
};
