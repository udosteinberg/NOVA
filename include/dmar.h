/*
 * DMA Remapping Unit (DMAR)
 *
 * Copyright (C) 2008-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "buddy.h"
#include "compiler.h"
#include "slab.h"
#include "types.h"
#include "x86.h"

class Pd;

class Dmar_context
{
    private:
        uint64 lo, hi;

    public:
        ALWAYS_INLINE
        inline bool present() const { return lo & 1; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(lo) & ~0xfff; }

        ALWAYS_INLINE
        inline void set (uint64 h, uint64 l) { hi = h; lo = l; clflush (this); }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }
};

class Dmar
{
    private:
        mword const reg_base;
        mword       frr_base;
        mword       tlb_base;
        unsigned    frr_count;
        Dmar *      next;

        static Slab_cache       cache;
        static Dmar *           list;
        static Dmar_context *   root;

        enum Reg
        {
            REG_VER     = 0x0,
            REG_CAP     = 0x8,
            REG_ECAP    = 0x10,
            REG_GCMD    = 0x18,
            REG_GSTS    = 0x1c,
            REG_RTADDR  = 0x20,
            REG_CCMD    = 0x28,
            REG_FSTS    = 0x34,
            REG_FECTL   = 0x38,
            REG_FEDATA  = 0x3c,
            REG_FEADDR  = 0x40,
        };

        enum Tlb
        {
            REG_IVA     = 0x0,
            REG_IOTLB   = 0x8,
        };

        template <typename T>
        ALWAYS_INLINE
        inline T read (Reg reg)
        {
            return *reinterpret_cast<T volatile *>(reg_base + reg);
        }

        template <typename T>
        ALWAYS_INLINE
        inline void write (Reg reg, T val)
        {
            *reinterpret_cast<T volatile *>(reg_base + reg) = val;
        }

        template <typename T>
        ALWAYS_INLINE
        inline T read (Tlb tlb)
        {
            return *reinterpret_cast<T volatile *>(tlb_base + tlb);
        }

        template <typename T>
        ALWAYS_INLINE
        inline void write (Tlb tlb, T val)
        {
            *reinterpret_cast<T volatile *>(tlb_base + tlb) = val;
        }

        ALWAYS_INLINE
        inline void read (unsigned frr, uint64 &hi, uint64 &lo)
        {
            lo = *reinterpret_cast<uint64 volatile *>(frr_base + frr * 16);
            hi = *reinterpret_cast<uint64 volatile *>(frr_base + frr * 16 + 8);
            *reinterpret_cast<uint64 volatile *>(frr_base + frr * 16 + 8) = 1ull << 63;
        }

        void enable();
        void fault_handler();

    public:
        Dmar (Paddr);

        void assign (unsigned, unsigned, unsigned, Pd *);

        ALWAYS_INLINE
        static inline void enable_all()
        {
            for (Dmar *dmar = list; dmar; dmar = dmar->next)
                dmar->enable();
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }

        REGPARM (1)
        static void vector (unsigned) asm ("msi_vector");
};
