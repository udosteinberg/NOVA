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

#include "ec_arch.hpp"
#include "fpu.hpp"
#include "space_dma.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), Kobject::alignment);

Pd::Pd() : Kobject (Kobject::Type::PD),
           dma_cache (sizeof (Space_dma), Kobject::alignment),
           gst_cache (sizeof (Space_gst), Kobject::alignment),
           hst_cache (sizeof (Space_hst), Kobject::alignment),
           msr_cache (sizeof (Space_msr), Kobject::alignment),
           obj_cache (sizeof (Space_obj), Kobject::alignment),
           pio_cache (sizeof (Space_pio), Kobject::alignment),
           fpu_cache (sizeof (Fpu), 16)
{
    trace (TRACE_CREATE, "PD:%p created", static_cast<void *>(this));
}

Space_obj *Pd::create_obj (Status &s, Space_obj *obj, unsigned long sel)
{
    if (EXPECT_FALSE (!attach (Kobject::Subtype::OBJ))) {
        s = Status::ABORTED;
        return nullptr;
    }

    auto const o { Space_obj::create (s, obj_cache, this) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_sp::DEFINED_OBJ)))) == Status::SUCCESS))
            return space_obj = o;

        o->destroy (obj_cache);
    }

    detach (Kobject::Subtype::OBJ);

    return nullptr;
}

Space_hst *Pd::create_hst (Status &s, Space_obj *obj, unsigned long sel)
{
    if (EXPECT_FALSE (!attach (Kobject::Subtype::HST))) {
        s = Status::ABORTED;
        return nullptr;
    }

    auto const o { Space_hst::create (s, hst_cache, this) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_sp::DEFINED_HST)))) == Status::SUCCESS))
            return space_hst = o;

        o->destroy (hst_cache);
    }

    detach (Kobject::Subtype::HST);

    return nullptr;
}

Space_gst *Pd::create_gst (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const o { Space_gst::create (s, gst_cache, this) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_sp::DEFINED_GST)))) == Status::SUCCESS))
            return o;

        o->destroy (gst_cache);
    }

    return nullptr;
}

Space_dma *Pd::create_dma (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const o { Space_dma::create (s, dma_cache, this) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_sp::DEFINED_DMA)))) == Status::SUCCESS))
            return o;

        o->destroy (dma_cache);
    }

    return nullptr;
}

Space_pio *Pd::create_pio (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const a { attach (Kobject::Subtype::PIO) };

    auto const o { Space_pio::create (s, pio_cache, this, a) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_sp::DEFINED_PIO)))) == Status::SUCCESS))
            return a ? space_pio = o : o;

        o->destroy (pio_cache);
    }

    if (a)
        detach (Kobject::Subtype::PIO);

    return nullptr;
}

Space_msr *Pd::create_msr (Status &s, Space_obj *obj, unsigned long sel)
{
    auto const o { Space_msr::create (s, msr_cache, this) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_sp::DEFINED_MSR)))) == Status::SUCCESS))
            return o;

        o->destroy (msr_cache);
    }

    return nullptr;
}

Pd *Pd::create_pd (Status &s, Space_obj *obj, unsigned long sel, unsigned prm)
{
    auto const o { Pd::create (s) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, prm))) == Status::SUCCESS))
            return o;

        o->destroy();
    }

    return nullptr;
}

Ec *Pd::create_ec (Status &s, Space_obj *obj, unsigned long sel, Pd *pd, unsigned cpu, uintptr_t utcb, uintptr_t sp, uintptr_t eb, uint8 flg)
{
    auto const o { flg & BIT (1) ? Ec::create (s, pd, flg & BIT (2), cpu, eb, flg & BIT (0)) : Ec::create (s, pd, flg & BIT (2), cpu, eb, flg & BIT (0), utcb, sp) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_ec::DEFINED)))) == Status::SUCCESS))
            return o;

        o->destroy();
    }

    return nullptr;
}

Sc *Pd::create_sc (Status &s, Space_obj *obj, unsigned long sel, Ec *ec, unsigned cpu, uint16 budget, uint8 prio, uint16 cos)
{
    auto const o { Sc::create (s, cpu, ec, budget, prio, cos) };

    if (EXPECT_TRUE (o)) {

        if (EXPECT_TRUE ((s = obj->insert (sel, Capability (o, std::to_underlying (Capability::Perm_sc::DEFINED)))) == Status::SUCCESS))
            return o;

        o->destroy();
    }

    return nullptr;
}
