/*
 * Protection Domain (PD)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

class Ec;
class Pt;
class Sc;

class Pd final : public Kobject
{
    private:
        Refptr<Pd> const    pd;         // Owner PD
        Atomic<unsigned>    spaces { 0 };

        Atomic<Space_obj *, __ATOMIC_ACQUIRE, __ATOMIC_RELEASE> space_obj { nullptr };
        Atomic<Space_hst *, __ATOMIC_ACQUIRE, __ATOMIC_RELEASE> space_hst { nullptr };
        Atomic<Space_pio *, __ATOMIC_ACQUIRE, __ATOMIC_RELEASE> space_pio { nullptr };

        explicit Pd (Refptr<Pd> &);

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: PD %p collected", static_cast<void *>(this));
        }

        auto attach (Kobject::Subtype s) { return !spaces.test_and_set (BIT (std::to_underlying (s))); }
        void detach (Kobject::Subtype s) { spaces &= ~BIT (std::to_underlying (s)); }

    public:
        Slab_cache pd_cache;
        Slab_cache ec_cache;
        Slab_cache sc_cache;
        Slab_cache pt_cache;

        Slab_cache obj_cache;
        Slab_cache hst_cache;
        Slab_cache gst_cache;
        Slab_cache dma_cache;
        Slab_cache pio_cache;
        Slab_cache msr_cache;
        Slab_cache fpu_cache;

        static Pd nova;

        static inline constinit Pd *root { nullptr };

        [[nodiscard]] static Pd *create (Status &, Pd *);

        void destroy() override final;

        Space_obj *get_obj() const { return space_obj; }
        Space_hst *get_hst() const { return space_hst; }
        Space_pio *get_pio() const { return space_pio; }

        Space_dma *create_dma (Status &, Space_obj *, unsigned long);
        Space_gst *create_gst (Status &, Space_obj *, unsigned long);
        Space_hst *create_hst (Status &, Space_obj *, unsigned long);
        Space_msr *create_msr (Status &, Space_obj *, unsigned long);
        Space_obj *create_obj (Status &, Space_obj *, unsigned long);
        Space_pio *create_pio (Status &, Space_obj *, unsigned long);

        Pd *create_pd (Status &, Space_obj *, unsigned long, unsigned);
        Ec *create_ec (Status &, Space_obj *, unsigned long, cpu_t, uintptr_t, uintptr_t, uintptr_t, uint8_t);
        Sc *create_sc (Status &, Space_obj *, unsigned long, Ec *, cpu_t, uint16_t, uint8_t, uint16_t);
        Pt *create_pt (Status &, Space_obj *, unsigned long, Ec *, uintptr_t);
};
