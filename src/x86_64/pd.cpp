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

#include "ec_arch.hpp"
#include "fpu.hpp"
#include "space_dma.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"
#include "stdio.hpp"

Refptr<Pd> nullref;

INIT_PRIORITY (PRIO_SLAB) Pd Pd::nova { nullref };

Pd::Pd (Refptr<Pd> &ref_pd) : Kobject   { Kobject::Type::PD, Kobject::Subtype::PD },
                              pd        { std::move (ref_pd) },
                              pd_cache  { sizeof (Pd),        Kobject::alignment },
                              ec_cache  { sizeof (Ec),        Kobject::alignment },
                              obj_cache { sizeof (Space_obj), Kobject::alignment },
                              hst_cache { sizeof (Space_hst), Kobject::alignment },
                              gst_cache { sizeof (Space_gst), Kobject::alignment },
                              dma_cache { sizeof (Space_dma), Kobject::alignment },
                              pio_cache { sizeof (Space_pio), Kobject::alignment },
                              msr_cache { sizeof (Space_msr), Kobject::alignment },
                              fpu_cache { sizeof (Fpu), 16 }
{
    // Pd::nova needs refcount > 0 so that other objects can acquire a reference to it
    if (!pd) [[unlikely]]
        ref_inc();

    trace (TRACE_CREATE, "PD:%p created (PD:%p)", static_cast<void *>(this), static_cast<void *>(pd));
}

Pd *Pd::create (Status &s, Pd *pd)
{
    // Acquire reference
    Refptr<Pd> ref_pd { pd };

    // Failed to acquire reference
    if (!ref_pd) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new PD object
        auto const obj { new (ref_pd->pd_cache) Pd { ref_pd } };

        // If creation succeeded, then reference must have been consumed
        if (obj) [[likely]] {
            assert (!ref_pd);
            return obj;
        }

        // Failed to create PD object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Pd::destroy()
{
    auto &cache { pd->pd_cache };

    this->~Pd();

    operator delete (this, cache);
}

Space_obj *Pd::create_obj (Status &s, Space_obj *obj, unsigned long sel)
{
    if (!attach (Kobject::Subtype::OBJ)) [[unlikely]] {
        s = Status::ABORTED;
        return nullptr;
    }

    auto const o { Space_obj::create (s, this) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, std::to_underlying (Capability::Perm_sp::DEFINED_OBJ) })) == Status::SUCCESS) [[likely]]
            return space_obj = o;

        o->destroy();
    }

    detach (Kobject::Subtype::OBJ);

    return nullptr;
}

Space_hst *Pd::create_hst (Status &s, Space_obj *obj, unsigned long sel)
{
    if (!attach (Kobject::Subtype::HST)) [[unlikely]] {
        s = Status::ABORTED;
        return nullptr;
    }

    auto const o { Space_hst::create (s, this) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, std::to_underlying (Capability::Perm_sp::DEFINED_HST) })) == Status::SUCCESS) [[likely]]
            return space_hst = o;

        o->destroy();
    }

    detach (Kobject::Subtype::HST);

    return nullptr;
}

Space_gst *Pd::create_gst (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const o { Space_gst::create (s, this) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, std::to_underlying (Capability::Perm_sp::DEFINED_GST) })) == Status::SUCCESS) [[likely]]
            return o;

        o->destroy();
    }

    return nullptr;
}

Space_dma *Pd::create_dma (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const o { Space_dma::create (s, this) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, std::to_underlying (Capability::Perm_sp::DEFINED_DMA) })) == Status::SUCCESS) [[likely]]
            return o;

        o->destroy();
    }

    return nullptr;
}

Space_pio *Pd::create_pio (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const a { attach (Kobject::Subtype::PIO) };

    auto const o { Space_pio::create (s, this, a) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, std::to_underlying (Capability::Perm_sp::DEFINED_PIO) })) == Status::SUCCESS) [[likely]]
            return a ? space_pio = o : o;

        o->destroy();
    }

    if (a)
        detach (Kobject::Subtype::PIO);

    return nullptr;
}

Space_msr *Pd::create_msr (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const o { Space_msr::create (s, this) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, std::to_underlying (Capability::Perm_sp::DEFINED_MSR) })) == Status::SUCCESS) [[likely]]
            return o;

        o->destroy();
    }

    return nullptr;
}

Pd *Pd::create_pd (Status &s, Space_obj *obj, unsigned long sel, unsigned prm)
{
    auto const o { Pd::create (s, this) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, prm })) == Status::SUCCESS) [[likely]]
            return o;

        o->destroy();
    }

    return nullptr;
}

Ec *Pd::create_ec (Status &s, Space_obj *obj, unsigned long sel, cpu_t cpu, uintptr_t evt, uintptr_t sp, uintptr_t hva, uint8_t flg)
{
    auto const o { (flg & BIT (0) ? Ec::create_gst : Ec::create_hst) (s, this, flg & BIT (1), flg & BIT (2), cpu, evt, sp, hva) };

    if (o) [[likely]] {

        if ((s = obj->insert (sel, Capability { o, std::to_underlying (Capability::Perm_ec::DEFINED) })) == Status::SUCCESS) [[likely]]
            return o;

        o->destroy();
    }

    return nullptr;
}
