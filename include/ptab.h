/*
 * IA32 Page Table
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

        Ptab *walk (mword, unsigned long, mword = 0);

    public:
        ALWAYS_INLINE
        inline void set (Paddr v) { val = v; }

        ALWAYS_INLINE
        static inline mword hw_attr (mword a) { return a ? a | ATTR_USER | ATTR_PRESENT : 0; }

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

        size_t lookup (mword, Paddr &);

        bool sync_from (Ptab *, mword);

        size_t sync_master (mword virt);

        void sync_master_range (mword s_addr, mword e_addr);

        void *remap (Paddr phys);

        bool update (mword, mword, Paddr, mword, bool = false);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }
};
