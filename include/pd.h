/*
 * Protection Domain
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
#include "crd.h"
#include "kobject.h"
#include "space_io.h"
#include "space_mem.h"
#include "space_obj.h"

class Pd : public Kobject, public Space_mem, public Space_io, public Space_obj
{
    private:
        static Slab_cache cache;

        WARN_UNUSED_RESULT
        mword clamp (mword,   mword &, mword, mword);

        WARN_UNUSED_RESULT
        mword clamp (mword &, mword &, mword, mword, mword);

    public:
        static Pd *current CPULOCAL_HOT;
        static Pd kern, root;

        INIT
        Pd (Pd *);

        Pd (Pd *, mword);

        ALWAYS_INLINE HOT
        inline void make_current()
        {
            if (EXPECT_FALSE (flush.chk (Cpu::id))) {

                flush.clr (Cpu::id);

#if 0
                if (ept.addr())
                    ept.flush();    // XXX: Fix INVEPT
#endif

            } else if (EXPECT_TRUE (current == this))
                return;

            current = this;

            cpu_ptab (Cpu::id)->make_current();
        }

        ALWAYS_INLINE
        static inline Pd *remote (unsigned c)
        {
            return *reinterpret_cast<volatile typeof current *>(reinterpret_cast<mword>(&current) - CPULC_ADDR + CPUGL_ADDR + c * PAGE_SIZE);
        }

        ALWAYS_INLINE
        static inline void init_cpu_ptab (Ptab *ptab)
        {
            kern.percpu[Cpu::id] = ptab;
        }

        template <typename>
        void delegate (Pd *, mword, mword, mword, mword, mword = 0);

        template <typename>
        void revoke (mword, mword, mword, bool);

        void delegate_item  (Pd *, Crd, Crd &, mword = 0);
        void delegate_items (Pd *, Crd, mword *, mword *, unsigned long);

        void revoke_crd (Crd, bool);
        void lookup_crd (Crd &);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
