/*
 * Kernel Memory Management
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

#include "compiler.hpp"
#include "memory.hpp"
#include "types.hpp"

class Kmem final
{
    private:
        static inline uintptr_t offset { 0 };

        static inline auto phys_to_virt (uintptr_t p) { return p + offset; }
        static inline auto virt_to_phys (uintptr_t v) { return v - offset; }

    public:
        static inline void init (uintptr_t o) { offset = o; }

        NONNULL static inline auto phys_to_ptr (Paddr p) { return reinterpret_cast<void *>(phys_to_virt (p)); }
        NONNULL static inline auto ptr_to_phys (void *p) { return virt_to_phys (reinterpret_cast<uintptr_t>(p)); }

        NONNULL static inline auto sym_to_virt (void *p) { return reinterpret_cast<uintptr_t>(p) + OFFSET; }
        NONNULL static inline auto sym_to_phys (void *p) { return virt_to_phys (sym_to_virt (p)); }

        // Convert CPULOCAL pointer to global alias pointer
        template <typename T>
        NONNULL static inline auto loc_to_glob (T *p, unsigned cpu)
        {
            return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(p) - MMAP_CPU_DATA + MMAP_GLB_DATA + cpu * PAGE_SIZE);
        }
};
