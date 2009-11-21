/*
 * IA32 Page Table
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "assert.h"
#include "buddy.h"
#include "compiler.h"
#include "paging.h"
#include "types.h"

class Ptab : public Paging
{
    private:
        static Paddr remap_addr;

        ALWAYS_INLINE
        static inline void flush()
        {
            mword cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3));
        }

        Ptab *walk (mword, unsigned long, bool);

    public:
        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        ALWAYS_INLINE
        static inline Ptab *master()
        {
            extern Ptab _master_l;
            return &_master_l;
        }

        ALWAYS_INLINE
        static inline Ptab *current()
        {
            mword addr;
            asm volatile ("mov %%cr3, %0" : "=r" (addr));
            return static_cast<Ptab *>(Buddy::phys_to_ptr (addr));
        }

        ALWAYS_INLINE
        inline void make_current()
        {
            assert (this);
            asm volatile ("mov %0, %%cr3" : : "r" (Buddy::ptr_to_phys (this)) : "memory");
        }

        void insert (mword, mword, mword, Paddr);
        void remove (mword, mword);

        bool lookup (mword, size_t &, Paddr &);

        bool sync_from (Ptab *, mword);

        size_t sync_master (mword virt);

        void sync_master_range (mword s_addr, mword e_addr);

        void *remap (Paddr phys);
};
