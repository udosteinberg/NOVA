/*
 * Buddy Allocator
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "compiler.h"
#include "extern.h"
#include "memory.h"
#include "spinlock.h"
#include "types.h"

class Buddy
{
    private:
        class Block
        {
            public:
                Block *         prev;
                Block *         next;
                unsigned short  ord;
                unsigned short  tag;

                enum {
                    Used  = 0,
                    Free  = 1
                };
        };

        Spinlock        lock;
        signed long     max_idx;
        signed long     min_idx;
        mword           base;
        mword           order;
        Block *         index;
        Block *         head;

        ALWAYS_INLINE
        inline signed long block_to_index (Block *b)
        {
            return b - index;
        }

        ALWAYS_INLINE
        inline Block *index_to_block (signed long i)
        {
            return index + i;
        }

        ALWAYS_INLINE
        inline signed long page_to_index (mword l_addr)
        {
            return l_addr / PAGE_SIZE - base / PAGE_SIZE;
        }

        ALWAYS_INLINE
        inline mword index_to_page (signed long i)
        {
            return base + i * PAGE_SIZE;
        }

        ALWAYS_INLINE
        inline mword virt_to_phys (mword virt)
        {
            return virt - reinterpret_cast<mword>(&OFFSET);
        }

        ALWAYS_INLINE
        inline mword phys_to_virt (mword phys)
        {
            return phys + reinterpret_cast<mword>(&OFFSET);
        }

    public:
        enum Fill
        {
            NOFILL,
            FILL_0,
            FILL_1
        };

        static Buddy allocator;

        INIT
        Buddy (mword phys, mword virt, mword f_addr, size_t size);

        void *alloc (unsigned short ord, Fill fill = NOFILL);

        void free (mword addr);

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
