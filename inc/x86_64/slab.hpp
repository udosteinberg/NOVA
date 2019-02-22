/*
 * Slab Allocator
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

#include "initprio.hpp"
#include "spinlock.hpp"

class Slab_cache final
{
    private:
        struct Slab;

        uint16 const    bsz;                    // Buffer size
        uint16 const    bps;                    // Buffers per Slab
        Slab *          curr    { nullptr };    // Current (Partial) Slab
        Slab *          head    { nullptr };    // Head of Slab List
        Spinlock        lock;                   // Allocator Spinlock

    public:
        [[nodiscard]]
        void *alloc();

        void free (void *);

        Slab_cache (size_t, size_t);
};
