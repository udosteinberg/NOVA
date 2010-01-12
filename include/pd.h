/*
 * Protection Domain
 *
 * Copyright (C) 2007-2010, Udo Steinberg <udo@hypervisor.org>
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
#include "slab.h"
#include "space_io.h"
#include "space_mem.h"
#include "space_obj.h"
#include "types.h"

class Pd : public Kobject, public Space_mem, public Space_io, public Space_obj
{
    private:
        static Slab_cache cache;

        static void revoke_mem  (mword, size_t, bool);
        static void revoke_io   (mword, size_t, bool);
        static void revoke_obj  (mword, size_t, bool);
        static void revoke_pt   (Capability, mword, bool);

        mword clamp (mword &, mword, mword, mword);
        mword clamp (mword &, mword &, mword, mword, mword);

    public:
        // Current PD
        static Pd *current CPULOCAL_HOT;

        // Kernel PD
        static Pd kern;

        // Root PD
        static Pd *root;

        INIT
        Pd();

        Pd (unsigned);

        ALWAYS_INLINE
        inline bool privileged() const { return this == root && current == root; }

        ALWAYS_INLINE HOT
        inline void make_current()
        {
            if (EXPECT_TRUE (current == this))
                return;

            current = this;

            cpu_ptab (Cpu::id)->make_current();
        }

        ALWAYS_INLINE
        static inline void init_cpu_ptab (Ptab *ptab)
        {
            kern.percpu[Cpu::id] = ptab;
        }

        void delegate_items (Crd, mword *, unsigned long);
        void delegate_mem   (mword, mword, mword, mword);
        void delegate_io    (mword, mword);
        void delegate_obj   (mword, mword, mword);
        void delegate       (Crd, Crd, mword = 0);

        static void revoke (Crd, bool);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
