/*
 * Virtual Translation Lookaside Buffer (VTLB)
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

#include "compiler.h"
#include "pte.h"

class Exc_regs;

class Vtlb : public Pte<Vtlb, mword, 2, 10>
{
    private:
        enum
        {
            ERR_P   = 1UL << 0,
            ERR_W   = 1UL << 1,
            ERR_U   = 1UL << 2,
        };

        ALWAYS_INLINE
        inline bool global() const { return val & TLB_G; }

        ALWAYS_INLINE
        inline bool frag() const { return val & TLB_F; }

        ALWAYS_INLINE
        static inline size_t walk (Exc_regs *, mword, mword &, mword &, mword &);

        void flush_ptab (unsigned);

    public:
        enum
        {
            TLB_P   = 1UL << 0,
            TLB_W   = 1UL << 1,
            TLB_U   = 1UL << 2,
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

        void flush_addr (mword, unsigned long);
        void flush (unsigned, unsigned long);

        static Reason miss (Exc_regs *, mword, mword &);
        static bool load_pdpte (Exc_regs *, uint64 (&)[4]);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::NOFILL); }
};
