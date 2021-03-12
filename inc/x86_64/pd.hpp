/*
 * Protection Domain (PD)
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

#include "atomic.hpp"
#include "kobject.hpp"
#include "status.hpp"
#include "std.hpp"

class Space_dma;
class Space_gst;
class Space_hst;
class Space_msr;
class Space_obj;
class Space_pio;

class Pd final : public Kobject
{
    private:
        Atomic<unsigned>    spaces      { 0 };
        Atomic<Space_obj *> space_obj   { nullptr };
        Atomic<Space_hst *> space_hst   { nullptr };
        Atomic<Space_pio *> space_pio   { nullptr };

        Slab_cache dma_cache;
        Slab_cache gst_cache;
        Slab_cache hst_cache;
        Slab_cache msr_cache;
        Slab_cache obj_cache;
        Slab_cache pio_cache;

        Pd();

        inline auto attach (Kobject::Subtype s) { return !spaces.test_and_set (BIT (std::to_underlying (s))); }
        inline void detach (Kobject::Subtype s) { spaces &= ~BIT (std::to_underlying (s)); }

        static Slab_cache cache;

    public:
        Slab_cache fpu_cache;

        static inline Pd *root { nullptr };

        [[nodiscard]] static inline auto create (Status &s)
        {
            auto const pd { new (cache) Pd };

            if (EXPECT_FALSE (!pd))
                s = Status::MEM_OBJ;

            return pd;
        }

        inline void destroy() { operator delete (this, cache); }

        inline Space_obj *get_obj() const { return space_obj; }
        inline Space_hst *get_hst() const { return space_hst; }
        inline Space_pio *get_pio() const { return space_pio; }

        Space_dma *create_dma (Status &, Space_obj *, unsigned long);
        Space_gst *create_gst (Status &, Space_obj *, unsigned long);
        Space_hst *create_hst (Status &, Space_obj *, unsigned long);
        Space_msr *create_msr (Status &, Space_obj *, unsigned long);
        Space_obj *create_obj (Status &, Space_obj *, unsigned long);
        Space_pio *create_pio (Status &, Space_obj *, unsigned long);

        static Pd *create_pd (Status &, Space_obj *, unsigned long, unsigned);
};
