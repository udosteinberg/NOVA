/*
 * PIO Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "bitmap_pio.hpp"
#include "space_hst.hpp"

class Space_pio final : public Space
{
    private:
        Bitmap_pio *const bmp;
        Space_hst * const hst;

        static Space_pio nova;

        Space_pio();

        inline Space_pio (Pd *p, Bitmap_pio *b, Space_hst *h) : Space (Kobject::Subtype::PIO, p), bmp (b), hst (h)
        {
            if (hst)
                hst->update (MMAP_SPC_PIO, Kmem::ptr_to_phys (bmp), 1, Paging::R, Memattr::ram());
        }

        inline ~Space_pio()
        {
            if (hst)
                hst->update (MMAP_SPC_PIO, 0, 1, Paging::NONE, Memattr::ram());

            delete bmp;
        }

        [[nodiscard]] Paging::Permissions lookup (unsigned long) const;

        void update (unsigned long, Paging::Permissions);

    public:
        [[nodiscard]] Status delegate (Space_pio const *, unsigned long, unsigned long, unsigned, unsigned);

        [[nodiscard]] inline auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        [[nodiscard]] static inline Space_pio *create (Status &s, Slab_cache &cache, Pd *pd, bool a)
        {
            auto const hst { pd->get_hst() };

            if (EXPECT_FALSE (!hst)) {
                s = Status::ABORTED;
                return nullptr;
            }

            auto const bmp { new Bitmap_pio };

            if (EXPECT_TRUE (bmp)) {

                auto const pio { new (cache) Space_pio (pd, bmp, a ? hst : nullptr) };

                if (EXPECT_TRUE (pio))
                    return pio;

                delete bmp;
            }

            s = Status::MEM_OBJ;

            return nullptr;
        }

        inline void destroy (Slab_cache &cache) { operator delete (this, cache); }

        static void user_access (uint64_t base, size_t size, Paging::Permissions p)
        {
            for (unsigned i { 0 }; i < size; i++)       // FIXME: Optimize
                nova.update (base + i, p);
        }
};
