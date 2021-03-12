/*
 * Guest Memory Space
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

#include "space_gst.hpp"

Space_gst *Space_gst::create (Status &s, Pd *pd)
{
    // Acquire reference
    Refptr<Pd> ref_pd { pd };

    // Failed to acquire reference
    if (!ref_pd) [[unlikely]]
        s = Status::ABORTED;

    else {

        // Create new GST object
        auto const obj { new (ref_pd->gst_cache) Space_gst { ref_pd } };

        // If creation succeeded, then reference must have been consumed
        if (obj) [[likely]] {

            assert (!ref_pd);

            if (obj->eptp.root_init()) [[likely]]
                return obj;

            operator delete (obj, ref_pd->gst_cache);
        }

        // Failed to create GST object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Space_gst::destroy()
{
    auto &cache { get_pd()->gst_cache };

    this->~Space_gst();

    operator delete (this, cache);
}
