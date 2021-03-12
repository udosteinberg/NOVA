/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "bits.hpp"
#include "memattr.hpp"
#include "memory.hpp"
#include "paging.hpp"
#include "space.hpp"

template<typename T> class Space_mem : public Space
{
    protected:
        Space_mem (Kobject::Subtype s) : Space { s } {}

        Space_mem (Kobject::Subtype s, Refptr<Pd> &p) : Space { s, p } {}

        static void access_ctrl (T &mem, uint64_t phys, size_t size, Paging::Permissions perm, Memattr attr)
        {
            for (unsigned o; size; size -= BITN (o), phys += BITN (o))
                mem.update (phys, phys, (o = aligned_order (size, phys)) - PAGE_BITS, perm, attr);
        }

    public:
        Status delegate (Space_hst const *, unsigned long, unsigned long, unsigned, unsigned, Memattr);
};
