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

class Ept;

class Cte
{
    private:
        uint64 lo;
        uint64 hi;

        enum
        {
            CTE_P       = 1ul << 0,
            CTE_FPD     = 1ul << 1,
            CTE_T       = 1ul << 2,
            CTE_EH      = 1ul << 4,
            CTE_ALH     = 1ul << 5,
            CTE_ADDR    = ~((1ul << 12) - 1)
        };

    public:
        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        void set (unsigned, unsigned, Ept *);
};

class Rte
{
    private:
        uint64 lo;
        uint64 hi;

        enum
        {
            RTE_P       = 1ul << 0,
            RTE_ADDR    = ~((1ul << 12) - 1)
        };

    public:
        static Rte *rtp;

        ALWAYS_INLINE
        inline bool present() const { return lo & RTE_P; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(lo) & RTE_ADDR; }

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        static void set (unsigned, unsigned, unsigned, Ept *);
};

class Dmar
{
    private:
        mword const reg_base;
        mword       frr_base;
        unsigned    frr_count;
        Dmar *      next;

        static Slab_cache cache;
        static Dmar *list;

        enum Register
        {
            DMAR_VER        = 0x0,
            DMAR_CAP        = 0x8,
            DMAR_ECAP       = 0x10,
            DMAR_GCMD       = 0x18,
            DMAR_GSTS       = 0x1c,
            DMAR_RTADDR     = 0x20,
            DMAR_CCMD       = 0x28,
            DMAR_FSTS       = 0x34,
            DMAR_FECTL      = 0x38,
            DMAR_FEDATA     = 0x3c,
            DMAR_FEADDR     = 0x40,
            DMAR_FEUADDR    = 0x44,
            DMAR_AFLOG      = 0x58,
            DMAR_PMEN       = 0x64,
            DMAR_PLMBASE    = 0x68,
            DMAR_PLMLIMIT   = 0x6c,
            DMAR_PHMBASE    = 0x70,
            DMAR_PHMLIMIT   = 0x78,
            DMAR_IQH        = 0x80,
            DMAR_IQT        = 0x88,
            DMAR_IQA        = 0x90,
            DMAR_ICS        = 0x9c,
            DMAR_IECTL      = 0xa0,
            DMAR_IEDATA     = 0xa4,
            DMAR_IEADDR     = 0xa8,
            DMAR_IEUADDR    = 0xac,
            DMAR_IRTA       = 0xb8
        };

        enum
        {
            GCMD_TE         = 1ul << 31,
            GCMD_SRTP       = 1ul << 30,
            FSTS_PFO        = 1ul << 0,     // Primary Fault Overflow
            FSTS_PPF        = 1ul << 1,     // Primary Pending Fault
            FSTS_AFO        = 1ul << 2,     // Advanced Fault Overflow
            FSTS_APF        = 1ul << 3,     // Advanced Pending Fault
            FSTS_IQE        = 1ul << 4,     // Invalidation Queue Error
            FSTS_ICE        = 1ul << 5,     // Invalidation Completion Error
            FSTS_ITE        = 1ul << 6,     // Invalidation Timeout Error
            FECTL_IM        = 1ul << 31,
            FECTL_IP        = 1ul << 30,
        };

        template <typename T>
        ALWAYS_INLINE
        inline T read (Register reg)
        {
            return *reinterpret_cast<T volatile *>(reg_base + reg);
        }

        template <typename T>
        ALWAYS_INLINE
        inline void write (Register reg, T val)
        {
            *reinterpret_cast<T volatile *>(reg_base + reg) = val;
        }

        ALWAYS_INLINE
        inline void read_frr (unsigned n, uint64 &hi, uint64 &lo)
        {
            hi = *reinterpret_cast<uint64 volatile *>(frr_base + n * 16 + 8);
            lo = *reinterpret_cast<uint64 volatile *>(frr_base + n * 16);
        }

        ALWAYS_INLINE
        inline void clear_frr (unsigned n)
        {
            *reinterpret_cast<uint64 volatile *>(frr_base + n * 16 + 8) = 1ull << 63;
        }

        void handle_faults();

    public:
        Dmar (Paddr);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }

        REGPARM (1)
        static void vector (unsigned) asm ("msi_vector");
};
