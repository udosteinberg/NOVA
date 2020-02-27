/*
 * Protection Domain
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

#include "crd.hpp"
#include "space_mem.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"

class Pd : public Kobject, public Refcount, public Space_mem, public Space_pio, public Space_obj
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

        Pd (Pd *);

        Pd (Pd *own, mword sel, mword a) : Kobject (PD, static_cast<Space_obj *>(own), sel, a) {}

        ALWAYS_INLINE HOT
        inline void make_current()
        {
            mword pcid = did;

            if (EXPECT_FALSE (htlb.chk (Cpu::id)))
                htlb.clr (Cpu::id);

            else {

                if (EXPECT_TRUE (current == this))
                    return;

                pcid |= static_cast<mword>(1ULL << 63);
            }

            current = this;

            loc[Cpu::id].make_current (Cpu::feature (Cpu::FEAT_PCID) ? pcid : 0);
        }

        ALWAYS_INLINE
        static inline Pd *remote (unsigned c)
        {
            return Atomic::load (*reinterpret_cast<decltype (current) *>(reinterpret_cast<mword>(&current) - CPU_LOCAL_DATA + HV_GLOBAL_CPUS + c * PAGE_SIZE));
        }

        ALWAYS_INLINE
        inline Space *subspace (Crd::Type t)
        {
            switch (t) {
                case Crd::MEM:  return static_cast<Space_mem *>(this);
                case Crd::PIO:  return static_cast<Space_pio *>(this);
                case Crd::OBJ:  return static_cast<Space_obj *>(this);
            }

            return nullptr;
        }

        template <typename>
        void delegate (Pd *, mword, mword, mword, mword, mword = 0);

        template <typename>
        void revoke (mword, mword, mword, bool);

        void xfer_items (Pd *, Crd, Crd, Xfer *, Xfer *, unsigned long);

        void xlt_crd (Pd *, Crd, Crd &);
        void del_crd (Pd *, Crd, Crd &, mword = 0, mword = 0);
        void rev_crd (Crd, bool);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
