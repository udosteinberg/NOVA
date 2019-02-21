/*
 * DMA Remapping Unit (DMAR)
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

#include "list.hpp"
#include "lowlevel.hpp"
#include "slab.hpp"

class Pd;

class Dmar_qi
{
    private:
        uint64 lo, hi;

    public:
        Dmar_qi (uint64 l = 0, uint64 h = 0) : lo (l), hi (h) {}
};

class Dmar_qi_ctx : public Dmar_qi
{
    public:
        Dmar_qi_ctx() : Dmar_qi (0x1 | 1UL << 4) {}
};

class Dmar_qi_tlb : public Dmar_qi
{
    public:
        Dmar_qi_tlb() : Dmar_qi (0x2 | 1UL << 4) {}
};

class Dmar_qi_iec : public Dmar_qi
{
    public:
        Dmar_qi_iec() : Dmar_qi (0x4 | 1UL << 4) {}
};

class Dmar_ctx
{
    private:
        uint64 lo, hi;

    public:
        ALWAYS_INLINE
        inline bool present() const { return lo & 1; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(lo) & ~PAGE_MASK; }

        ALWAYS_INLINE
        inline void set (uint64 h, uint64 l) { hi = h; lo = l; flush (this); }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return flush (Buddy::alloc (0, Buddy::Fill::BITS0), PAGE_SIZE); }
};

class Dmar_irt
{
    private:
        uint64 lo, hi;

    public:
        ALWAYS_INLINE
        inline void set (uint64 h, uint64 l) { hi = h; lo = l; flush (this); }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return flush (Buddy::alloc (0, Buddy::Fill::BITS0), PAGE_SIZE); }
};

class Dmar : public List<Dmar>
{
    private:
        mword const         reg_base;
        uint64              cap;
        uint64              ecap;
        Dmar_qi *           invq;
        unsigned            invq_idx;

        static Dmar_ctx *   ctx;
        static Dmar_irt *   irt;
        static uint32       gcmd;

        static Dmar *       list;
        static Slab_cache   cache;

        static unsigned const ord = 0;
        static unsigned const cnt = (PAGE_SIZE << ord) / sizeof (Dmar_qi);

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
            REG_IQH     = 0x80,
            REG_IQT     = 0x88,
            REG_IQA     = 0x90,
            REG_IRTA    = 0xb8,
        };

        enum Tlb
        {
            REG_IVA     = 0x0,
            REG_IOTLB   = 0x8,
        };

        enum Cmd
        {
            GCMD_SIRTP  = 1UL << 24,
            GCMD_IRE    = 1UL << 25,
            GCMD_QIE    = 1UL << 26,
            GCMD_SRTP   = 1UL << 30,
            GCMD_TE     = 1UL << 31,
        };

        ALWAYS_INLINE
        inline unsigned nfr() const { return static_cast<unsigned>(cap >> 40 & 0xff) + 1; }

        ALWAYS_INLINE
        inline mword fro() const { return static_cast<mword>(cap >> 20 & 0x3ff0) + reg_base; }

        ALWAYS_INLINE
        inline mword iro() const { return static_cast<mword>(ecap >> 4 & 0x3ff0) + reg_base; }

        ALWAYS_INLINE
        inline unsigned ir() const { return static_cast<unsigned>(ecap) & 0x8; }

        ALWAYS_INLINE
        inline unsigned qi() const { return static_cast<unsigned>(ecap) & 0x2; }

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
            return *reinterpret_cast<T volatile *>(iro() + tlb);
        }

        template <typename T>
        ALWAYS_INLINE
        inline void write (Tlb tlb, T val)
        {
            *reinterpret_cast<T volatile *>(iro() + tlb) = val;
        }

        ALWAYS_INLINE
        inline void read (unsigned frr, uint64 &hi, uint64 &lo)
        {
            lo = *reinterpret_cast<uint64 volatile *>(fro() + frr * 16);
            hi = *reinterpret_cast<uint64 volatile *>(fro() + frr * 16 + 8);
            *reinterpret_cast<uint64 volatile *>(fro() + frr * 16 + 8) = 1ULL << 63;
        }

        ALWAYS_INLINE
        inline void command (uint32 val)
        {
            write<uint32>(REG_GCMD, val);
            while ((read<uint32>(REG_GSTS) & val) != val)
                pause();
        }

        ALWAYS_INLINE
        inline void qi_submit (Dmar_qi const &q)
        {
            invq[invq_idx] = q;
            invq_idx = (invq_idx + 1) % cnt;
            write<uint64>(REG_IQT, invq_idx << 4);
        };

        ALWAYS_INLINE
        inline void qi_wait()
        {
            for (uint64 v = read<uint64>(REG_IQT); v != read<uint64>(REG_IQH); pause()) ;
        }

        ALWAYS_INLINE
        inline void flush_ctx()
        {
            if (qi()) {
                qi_submit (Dmar_qi_ctx());
                qi_submit (Dmar_qi_tlb());
                qi_wait();
            } else {
                write<uint64>(REG_CCMD, 1ULL << 63 | 1ULL << 61);
                while (read<uint64>(REG_CCMD) & (1ULL << 63))
                    pause();
                write<uint64>(REG_IOTLB, 1ULL << 63 | 1ULL << 60);
                while (read<uint64>(REG_IOTLB) & (1ULL << 63))
                    pause();
            }
        }

        void fault_handler();

    public:
        Dmar (Paddr);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void enable (unsigned flags)
        {
            if (!(flags & 1))
                gcmd &= ~GCMD_IRE;

            for (Dmar *dmar = list; dmar; dmar = dmar->next)
                dmar->command (gcmd);
        }

        ALWAYS_INLINE
        static inline void set_irt (unsigned i, unsigned rid, unsigned cpu, unsigned vec, unsigned trg)
        {
            irt[i].set (1ULL << 18 | rid, static_cast<uint64>(cpu) << 40 | vec << 16 | trg << 4 | 1);
        }

        ALWAYS_INLINE
        static bool ire() { return gcmd & GCMD_IRE; }

        void assign (unsigned long, Pd *);

        static void vector (unsigned) asm ("msi_vector");
};
