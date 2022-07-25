/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

template <typename T>
class Space_mem : public Space
{
    protected:
        Space_mem (Kobject::Subtype s) : Space { s } {}

        Space_mem (Kobject::Subtype s, Refptr<Pd> &p) : Space { s, p } {}

        static void user_access (T &mem, uint64_t addr, size_t size, bool a, Memattr ma)
        {
            for (unsigned o; size; size -= BITN (o), addr += BITN (o))
                mem.update (addr, addr, (o = static_cast<unsigned>(max_order (addr, size))) - PAGE_BITS, a ? Paging::Permissions (Paging::U | Paging::API) : Paging::NONE, ma);
        }

    public:
        Status delegate (Space_hst const *, unsigned long, unsigned long, unsigned, unsigned, Memattr);
};
