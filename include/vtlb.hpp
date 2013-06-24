/*
 * Virtual Translation Lookaside Buffer (VTLB)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "pte.hpp"
#include "user.hpp"

class Exc_regs;

class Vtlb : public Pte<Vtlb, uint32, 2, 10, false>
{
    private:
        ALWAYS_INLINE
        inline bool global() const { return val & TLB_G; }

        ALWAYS_INLINE
        inline bool frag() const { return val & TLB_F; }

        ALWAYS_INLINE
        static inline bool mark_pte (uint32 *pte, uint32 old, uint32 bits)
        {
            return EXPECT_TRUE ((old & bits) == bits) || User::cmp_swap (pte, old, old | bits) == ~0UL;
        }

        void flush_ptab (bool);

    public:
        static size_t gwalk (Exc_regs *, mword, mword &, mword &, mword &);
        static size_t hwalk (mword, mword &, mword &, mword &);

        enum
        {
            TLB_P   = 1UL << 0,
            TLB_W   = 1UL << 1,
            TLB_U   = 1UL << 2,
            TLB_UC  = 1UL << 4,
            TLB_A   = 1UL << 5,
            TLB_D   = 1UL << 6,
            TLB_S   = 1UL << 7,
            TLB_G   = 1UL << 8,
            TLB_F   = 1UL << 9,

            PTE_P   = TLB_P,
            PTE_S   = TLB_S,
        };

        enum Reason
        {
            SUCCESS,
            GLA_GPA,
            GPA_HPA
        };

        ALWAYS_INLINE
        inline Vtlb()
        {
            for (unsigned i = 0; i < 1UL << bpl(); i++)
                this[i].val = TLB_S;
        }

        void flush (mword);
        void flush (bool);

        static Reason miss (Exc_regs *, mword, mword &);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::NOFILL); }
};
