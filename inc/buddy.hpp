/*
 * Buddy Allocator
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "arch.hpp"
#include "compiler.hpp"
#include "extern.hpp"
#include "memory.hpp"
#include "spinlock.hpp"
#include "types.hpp"

class Buddy
{
    private:
        class Block
        {
            public:
                enum class Tag : uint16
                {
                    USED    = 0,
                    FREE    = 1,
                };

                Block *         prev;
                Block *         next;
                uint16          ord;
                Tag             tag;

        };

        Spinlock        lock;
        unsigned long   min_idx;
        unsigned long   max_idx;
        mword           base;
        Block *         index;
        Block *         head;

        static uint16 const order = PTE_BPL + 1;

        ALWAYS_INLINE
        inline unsigned long block_to_index (Block *b)
        {
            return b - index;
        }

        ALWAYS_INLINE
        inline Block *index_to_block (unsigned long i)
        {
            return index + i;
        }

        ALWAYS_INLINE
        inline unsigned long page_to_index (mword virt)
        {
            return (virt - base) / PAGE_SIZE;
        }

        ALWAYS_INLINE
        inline mword index_to_page (unsigned long i)
        {
            return base + i * PAGE_SIZE;
        }

        ALWAYS_INLINE
        inline mword virt_to_phys (mword virt)
        {
            return virt - OFFSET;
        }

        ALWAYS_INLINE
        inline mword phys_to_virt (mword phys)
        {
            return phys + OFFSET;
        }

    public:
        enum Fill
        {
            NOFILL,
            FILL_0,
            FILL_1
        };

        static Buddy allocator;

        Buddy (mword, mword, size_t);

        void *alloc (uint16, Fill = NOFILL);

        void free (mword);

        ALWAYS_INLINE
        static inline void *phys_to_ptr (Paddr phys)
        {
            return reinterpret_cast<void *>(allocator.phys_to_virt (static_cast<mword>(phys)));
        }

        ALWAYS_INLINE
        static inline mword ptr_to_phys (void *virt)
        {
            return allocator.virt_to_phys (reinterpret_cast<mword>(virt));
        }
};
