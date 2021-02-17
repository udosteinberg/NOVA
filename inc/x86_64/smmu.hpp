/*
 * System Memory Management Unit (Intel IOMMU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "buddy.hpp"
#include "cache.hpp"
#include "list.hpp"
#include "lowlevel.hpp"
#include "macros.hpp"
#include "memory.hpp"
#include "slab.hpp"
#include "space.hpp"
#include "spinlock.hpp"

class Pd;

class Smmu_qi
{
    private:
        uint64 lo, hi;

    public:
        Smmu_qi (uint64 l = 0, uint64 h = 0) : lo (l), hi (h) {}
};

class Smmu_qi_ctx : public Smmu_qi
{
    public:
        Smmu_qi_ctx() : Smmu_qi (0x1 | 1UL << 4) {}
};

class Smmu_qi_tlb : public Smmu_qi
{
    public:
        Smmu_qi_tlb() : Smmu_qi (0x2 | 1UL << 4) {}
};

class Smmu_qi_iec : public Smmu_qi
{
    public:
        Smmu_qi_iec() : Smmu_qi (0x4 | 1UL << 4) {}
};

class Smmu_ctx
{
    private:
        uint64 lo, hi;

    public:
        ALWAYS_INLINE
        inline bool present() const { return lo & 1; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(lo) & ~OFFS_MASK; }

        ALWAYS_INLINE
        inline void set (uint64 h, uint64 l) { hi = h; lo = l; Cache::data_clean (this); }

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            auto ptr = Buddy::alloc (0, Buddy::Fill::BITS0);

            // FIXME: We want to use Cache::data_clean (ptr, PAGE_SIZE) here, but per-CPU line size is not available yet
            if (EXPECT_TRUE (ptr))
                for (auto p = static_cast<char *>(ptr); p < static_cast<char *>(ptr) + PAGE_SIZE; p += 32)
                    Cache::data_clean (p);

            return ptr;
        }
};

class Smmu_irt
{
    private:
        uint64 lo, hi;

    public:
        ALWAYS_INLINE
        inline void set (uint64 h, uint64 l) { hi = h; lo = l; Cache::data_clean (this); }

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            auto ptr = Buddy::alloc (0, Buddy::Fill::BITS0);

            // FIXME: We want to use Cache::data_clean (ptr, PAGE_SIZE) here, but per-CPU line size is not available yet
            if (EXPECT_TRUE (ptr))
                for (auto p = static_cast<char *>(ptr); p < static_cast<char *>(ptr) + PAGE_SIZE; p += 32)
                    Cache::data_clean (p);

            return ptr;
        }
};

class Smmu final : public List<Smmu>
{
    private:
        Paddr const         phys_base;
        uintptr_t const     mmio_base;
        uint64              cap;
        uint64              ecap;
        unsigned            invq_idx            { 0 };
        Smmu_qi *           invq                { nullptr };
        Spinlock            lock;

        static        Slab_cache    cache;
        static inline uintptr_t     mmap        { MMAP_GLB_SMMU };
        static inline Smmu *        list        { nullptr };
        static inline Smmu_ctx *    ctx         { new Smmu_ctx };
        static inline Smmu_irt *    irt         { new Smmu_irt };

        static constexpr unsigned ord = 0;
        static constexpr unsigned cnt = (PAGE_SIZE << ord) / sizeof (Smmu_qi);

        enum class Register32
        {
            VER         = 0x000,        // Version
            GCMD        = 0x018,        // Global Command
            GSTS        = 0x01c,        // Global Status
            FSTS        = 0x034,        // Fault Status
            FECTL       = 0x038,        // Fault Event Control
            FEDATA      = 0x03c,        // Fault Event Data
            FEADDR      = 0x040,        // Fault Event Address
        };

        enum class Register64
        {
            CAP         = 0x008,        // Capability
            ECAP        = 0x010,        // Extended Capability
            RTADDR      = 0x020,        // Root Table Address
            CCMD        = 0x028,        // Context Command
            IQH         = 0x080,        // Invalidation Queue Head
            IQT         = 0x088,        // Invalidation Queue Tail
            IQA         = 0x090,        // Invalidation Queue Address
            IRTA        = 0x0b8,        // Interrupt Remapping Table Address
        };

        enum class Tlb64
        {
            IVA         = 0x000,        // Invalidate Address
            IOTLB       = 0x008,        // IOTLB Invalidate
        };

        enum Cmd
        {
            CFI         = BIT (23),     // Compatibility Format Interrupt
            IRTP        = BIT (24),     // Interrupt Remapping Table Pointer
            IRE         = BIT (25),     // Interrupt Remapping Enable
            QIE         = BIT (26),     // Queued Invalidation Enable
            WBF         = BIT (27),     // Write Buffer Flush
            AFL         = BIT (28),     // Advanced Fault Logging
            FL          = BIT (29),     // Fault Log
            RTP         = BIT (30),     // Root Table Pointer
            TE          = BIT (31),     // Translation Enable
        };

        inline bool has_qi() const { return ecap & BIT (1); }
        inline bool has_dt() const { return ecap & BIT (2); }
        inline bool has_ir() const { return ecap & BIT (3); }

        inline auto nfr() const { return static_cast<unsigned>(cap >> 40 & BIT_RANGE (7, 0)) + 1; }
        inline auto fro() const { return static_cast<unsigned>(cap >> 20 & BIT_RANGE (13, 4)); }
        inline auto iro() const { return static_cast<unsigned>(ecap >> 4 & BIT_RANGE (13, 4)); }

        inline auto read  (Register32 r) const    { return *reinterpret_cast<uint32 volatile *>(mmio_base         + static_cast<unsigned>(r)); }
        inline auto read  (Register64 r) const    { return *reinterpret_cast<uint64 volatile *>(mmio_base         + static_cast<unsigned>(r)); }
        inline auto read  (Tlb64      r) const    { return *reinterpret_cast<uint64 volatile *>(mmio_base + iro() + static_cast<unsigned>(r)); }

        inline void write (Register32 r, uint32 v) const { *reinterpret_cast<uint32 volatile *>(mmio_base         + static_cast<unsigned>(r)) = v; }
        inline void write (Register64 r, uint64 v) const { *reinterpret_cast<uint64 volatile *>(mmio_base         + static_cast<unsigned>(r)) = v; }
        inline void write (Tlb64      r, uint64 v) const { *reinterpret_cast<uint64 volatile *>(mmio_base + iro() + static_cast<unsigned>(r)) = v; }

        ALWAYS_INLINE
        inline void read (unsigned frr, uint64 &hi, uint64 &lo) const
        {
            lo = *reinterpret_cast<uint64 volatile *>(mmio_base + fro() + frr * 16);
            hi = *reinterpret_cast<uint64 volatile *>(mmio_base + fro() + frr * 16 + 8);
            *reinterpret_cast<uint64 volatile *>(mmio_base + fro() + frr * 16 + 8) = 1ULL << 63;
        }

        ALWAYS_INLINE
        inline void command (Cmd c) const
        {
            // Mask one-shot bits
            auto v = read (Register32::GSTS) & ~(Cmd::RTP | Cmd::FL | Cmd::WBF | Cmd::IRTP);

            // Only one bit may be modified at a time
            write (Register32::GCMD, v | c);

            // Wait for hardware to signal completion
            while (!(read (Register32::GSTS) & c))
                pause();
        }

        ALWAYS_INLINE
        inline void qi_submit (Smmu_qi const &q)
        {
            invq[invq_idx] = q;
            invq_idx = (invq_idx + 1) % cnt;
            write (Register64::IQT, invq_idx << 4);
        };

        ALWAYS_INLINE
        inline void qi_wait() const
        {
            for (uint64 v = read (Register64::IQT); v != read (Register64::IQH); pause()) ;
        }

        ALWAYS_INLINE
        inline void invalidate_ctx()
        {
            if (has_qi()) {
                qi_submit (Smmu_qi_ctx());
                qi_submit (Smmu_qi_tlb());
                qi_wait();
            } else {
                write (Register64::CCMD, 1ULL << 63 | 1ULL << 61);
                while (read (Register64::CCMD) & (1ULL << 63))
                    pause();
                write (Tlb64::IOTLB, 1ULL << 63 | 1ULL << 60);
                while (read (Tlb64::IOTLB) & (1ULL << 63))
                    pause();
            }
        }

        void fault_handler();

        void init();

    public:
        static inline bool use_ir { false };

        explicit Smmu (Paddr);

        ALWAYS_INLINE
        static inline void init_all()
        {
            for (auto l = list; l; l = l->next)
                l->init();
        }

        bool configure (Pd *, Space::Index, uintptr_t, bool = true);

        ALWAYS_INLINE
        static inline void set_irt (unsigned i, uint16 rid, uint32 aid, uint8 vec, bool trg)
        {
            irt[i].set (BIT (18) | rid, static_cast<uint64>(aid) << 40 | vec << 16 | trg << 4 | BIT (0));
        }

        static void vector (unsigned) asm ("msi_vector");

        static inline Smmu *lookup (Paddr p)
        {
            for (auto l = list; l; l = l->next)
                if (l->phys_base == p)
                    return l;

            return nullptr;
        }

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }
};
