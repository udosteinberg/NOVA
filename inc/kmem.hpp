/*
 * Kernel Memory Management
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "memory.hpp"
#include "types.hpp"

class Kmem
{
    private:
        static inline uintptr_t offset { 0 };

        static inline auto phys_to_virt (uintptr_t p) { return p + offset; }
        static inline auto virt_to_phys (uintptr_t v) { return v - offset; }

    public:
        static inline void init (uintptr_t o) { offset = o; }

        static inline auto phys_to_ptr (Paddr p) { return reinterpret_cast<void *>(phys_to_virt (p)); }
        static inline auto ptr_to_phys (void *p) { return virt_to_phys (reinterpret_cast<uintptr_t>(p)); }

        static inline auto sym_to_virt (void *p) { return reinterpret_cast<uintptr_t>(p) + OFFSET; }
        static inline auto sym_to_phys (void *p) { return virt_to_phys (sym_to_virt (p)); }
};
