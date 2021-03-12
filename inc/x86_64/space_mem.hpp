/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "paging.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_hst;

template<typename T> class Space_mem : public Space
{
    protected:
        static void access_ctrl (T &mem, uint64_t phys, size_t size, Paging::Permissions perm, Memattr attr)
        {
            bool inv { false };

            for (unsigned o; size; size -= BITN (o), phys += BITN (o))
                mem.update (phys, phys, (o = aligned_order (size, phys)) - PAGE_BITS, perm, attr, inv);
        }

    public:
        [[nodiscard]] static constexpr auto selectors() { return BITN (T::sbw()); }

        // Limit for user-accessible memory mappings
        [[nodiscard]] static constexpr auto user_boundary() { return selectors() << PAGE_BITS; }

        [[nodiscard]] static constexpr auto info_addr() { return user_boundary() - PAGE_SIZE (0) * 1; }
        [[nodiscard]] static constexpr auto utcb_addr() { return user_boundary() - PAGE_SIZE (0) * 2; }

        [[nodiscard]] Status delegate (Space_hst const *, unsigned long, unsigned long, unsigned, unsigned, Memattr);
};
