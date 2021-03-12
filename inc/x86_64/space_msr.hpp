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
#include "paging.hpp"
#include "space.hpp"

class Space_msr final : public Space
{
    private:
        Bitmap_msr *const bmp;

        static Space_msr nova;

        Space_msr();

        inline Space_msr (Pd *p, Bitmap_msr *b) : Space (Kobject::Subtype::MSR, p), bmp (b) {}

        inline ~Space_msr() { delete bmp; }

        [[nodiscard]] Paging::Permissions lookup (unsigned long) const;

        void update (unsigned long, Paging::Permissions);

    public:
        [[nodiscard]] Status delegate (Space_msr const *, unsigned long, unsigned long, unsigned, unsigned);

        [[nodiscard]] inline auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        [[nodiscard]] static inline Space_msr *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            auto const bmp { new Bitmap_msr };

            if (EXPECT_TRUE (bmp)) {

                auto const msr { new (cache) Space_msr (pd, bmp) };

                if (EXPECT_TRUE (msr))
                    return msr;

                delete bmp;
            }

            s = Status::MEM_OBJ;

            return nullptr;
        }

        inline void destroy (Slab_cache &cache) { operator delete (this, cache); }

        static void user_access (uint64 base, size_t size, bool a)
        {
            for (unsigned i { 0 }; i < size; i++)       // FIXME: Optimize
                nova.update (base + i, a ? Paging::Permissions (Paging::W | Paging::R) : Paging::NONE);
        }
};
