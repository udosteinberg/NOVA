/*
 * Kernel Memory Management
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

#include "memory.hpp"
#include "std.hpp"

class Kmem final
{
    private:
        static inline constinit uintptr_t offset asm ("__kmem_offset") { 0 };

        static auto phys_to_virt (uintptr_t p) { return p + offset; }
        static auto virt_to_phys (uintptr_t v) { return v - offset; }

    public:
        static auto sym_to_virt (void const *p) { return reinterpret_cast<uintptr_t>(p) + OFFSET; }
        static auto sym_to_phys (void const *p) { return virt_to_phys (sym_to_virt (p)); }
        static auto ptr_to_phys (void const *p) { return virt_to_phys (reinterpret_cast<uintptr_t>(p)); }

        /*
         * Convert a physical address to a T*
         * @pre     An instance of T is within its lifetime at phys_to_virt (p)
         * @post    Anchors the returned pointer's provenance to that live T
         */
        template<typename T> static auto phys_to_ptr (uintptr_t p)
        {
            return std::launder (reinterpret_cast<T *>(phys_to_virt (p)));
        }

        /*
         * Convert a CPULOCAL T* to a global T* aliasing CPU cpu's instance
         * @pre     CPU cpu's instance of T is within its lifetime at its MMAP_GLB_CPUS address
         * @post    Anchors the returned pointer's provenance to that live T
         */
        template<typename T> __attribute__((nonnull)) static auto loc_to_glb (unsigned cpu, T *t)
        {
            return std::launder (reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(t) - MMAP_CPU_DATA + MMAP_GLB_CPUS + cpu * PAGE_SIZE (0)));
        }
};
