/*
 * Kernel Memory Management
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
        static inline uintptr_t offset asm ("__kmem_offset") { 0 };

        static auto phys_to_virt (uintptr_t p) { return p + offset; }
        static auto virt_to_phys (uintptr_t v) { return v - offset; }

    public:
        NONNULL static auto sym_to_virt (void *p) { return reinterpret_cast<uintptr_t>(p) + OFFSET; }
        NONNULL static auto sym_to_phys (void *p) { return virt_to_phys (sym_to_virt (p)); }
        NONNULL static auto ptr_to_phys (void *p) { return virt_to_phys (reinterpret_cast<uintptr_t>(p)); }
        NONNULL static auto phys_to_ptr (uintptr_t p) { return reinterpret_cast<void *>(phys_to_virt (p)); }

        // Convert CPULOCAL pointer to global alias pointer
        template <typename T>
        NONNULL static auto loc_to_glob (T *p, unsigned cpu)
        {
            return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(p) - MMAP_CPU_DATA + MMAP_GLB_CPUS + cpu * PAGE_SIZE (0));
        }
};
