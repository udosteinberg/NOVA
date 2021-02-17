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
#include "lock_guard.hpp"
#include "lowlevel.hpp"
#include "macros.hpp"
#include "memory.hpp"
#include "sdid.hpp"
#include "slab.hpp"
#include "spinlock.hpp"
#include "std.hpp"

class Space_dma;

class Smmu_ctx final
{
    private:
        uint64 lo, hi;

    public:
        bool present() const { return lo & BIT (0); }

        auto addr() const { return static_cast<Paddr>(lo) & ~OFFS_MASK; }

        auto did() const { return static_cast<uint16>(hi >> 8); }

        void set (uint64 h, uint64 l) { hi = h; lo = l; Cache::data_clean (this); }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            auto const ptr { Buddy::alloc (0, Buddy::Fill::BITS0) };

            // FIXME: We want to use Cache::data_clean (ptr, PAGE_SIZE) here, but per-CPU line size is not available yet
            if (EXPECT_TRUE (ptr))
                for (auto p { static_cast<char *>(ptr) }; p < static_cast<char *>(ptr) + PAGE_SIZE; p += 32)
                    Cache::data_clean (p);

            return ptr;
        }
};

class Smmu_irt final
{
    private:
        uint64 lo, hi;

    public:
        void set (uint64 h, uint64 l) { hi = h; lo = l; Cache::data_clean (this); }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            auto const ptr { Buddy::alloc (0, Buddy::Fill::BITS0) };

            // FIXME: We want to use Cache::data_clean (ptr, PAGE_SIZE) here, but per-CPU line size is not available yet
            if (EXPECT_TRUE (ptr))
                for (auto p { static_cast<char *>(ptr) }; p < static_cast<char *>(ptr) + PAGE_SIZE; p += 32)
                    Cache::data_clean (p);

            return ptr;
        }
};

class Smmu final : public List<Smmu>
{
    private:
        enum class Register32 : unsigned
        {
            VER         = 0x000,            // Version
            GCMD        = 0x018,            // Global Command
            GSTS        = 0x01c,            // Global Status
            FSTS        = 0x034,            // Fault Status
            FECTL       = 0x038,            // Fault Event Control
            FEDATA      = 0x03c,            // Fault Event Data
            FEADDR      = 0x040,            // Fault Event Address
        };

        enum class Register64 : unsigned
        {
            CAP         = 0x008,            // Capability
            ECAP        = 0x010,            // Extended Capability
            RTADDR      = 0x020,            // Root Table Address
            CCMD        = 0x028,            // Context Command
            IQH         = 0x080,            // Invalidation Queue Head
            IQT         = 0x088,            // Invalidation Queue Tail
            IQA         = 0x090,            // Invalidation Queue Address
            IRTA        = 0x0b8,            // Interrupt Remapping Table Address
        };

        enum class Tlb64 : unsigned
        {
            IVA         = 0x000,            // Invalidate Address
            IOTLB       = 0x008,            // IOTLB Invalidate
        };

        enum Cmd
        {
            CFI         = BIT (23),         // Compatibility Format Interrupt
            IRTP        = BIT (24),         // Interrupt Remapping Table Pointer
            IRE         = BIT (25),         // Interrupt Remapping Enable
            QIE         = BIT (26),         // Queued Invalidation Enable
            WBF         = BIT (27),         // Write Buffer Flush
            AFL         = BIT (28),         // Advanced Fault Logging
            FL          = BIT (29),         // Fault Log
            RTP         = BIT (30),         // Root Table Pointer
            TE          = BIT (31),         // Translation Enable
        };

        enum Fault
        {
            PFO         = BIT (0),          // Primary Fault Overflow
            PPF         = BIT (1),          // Primary Pending Fault
            AFO         = BIT (2),          // Advanced Fault Overflow
            APF         = BIT (3),          // Advanced Pending Fault
            IQE         = BIT (4),          // Invalidation Queue Error
            ICE         = BIT (5),          // Invalidation Completion Error
            ITE         = BIT (6),          // Invalidation Timeout Error
        };

        /*
         * Invalidation Descriptor
         */
        class Inv_dsc
        {
            private:
                uint64 lo, hi;

            protected:
                enum class Type : unsigned  // Invalidation Type
                {
                    CTX = 1,                // Context-Cache
                    TLB = 2,                // IOTLB
                    DEV = 3,                // Device TLB
                    IEC = 4,                // Interrupt Entry Cache
                    INV = 5,                // Invalidation Wait
                };

            public:
                Inv_dsc (Type t, uint64 l, uint64 h = 0) : lo (l | std::to_underlying (t)), hi (h) {}
        };

        static_assert (__is_standard_layout (Inv_dsc) && sizeof (Inv_dsc) == 16);

        /*
         * CTX Invalidation Descriptor
         */
        struct Inv_dsc_ctx final : Inv_dsc
        {
            enum class Gran : unsigned      // Invalidation Granularity
            {
                GLOBAL  = 1,                // Global
                DOMAIN  = 2,                // Domain-Selective
                DEVICE  = 3,                // Device-Selective
            };

            Inv_dsc_ctx() : Inv_dsc (Type::CTX, std::to_underlying (Gran::GLOBAL) << 4) {}

            Inv_dsc_ctx (uint16 did) : Inv_dsc (Type::CTX, static_cast<uint64>(did) << 16 | std::to_underlying (Gran::DOMAIN) << 4) {}

            Inv_dsc_ctx (uint16 sid, uint16 did) : Inv_dsc (Type::CTX, static_cast<uint64>(sid) << 32 | static_cast<uint64>(did) << 16 | std::to_underlying (Gran::DEVICE) << 4) {}
        };

        static_assert (__is_standard_layout (Inv_dsc_ctx) && sizeof (Inv_dsc_ctx) == sizeof (Inv_dsc));

        /*
         * TLB Invalidation Descriptor
         */
        struct Inv_dsc_tlb final : Inv_dsc
        {
            enum class Gran : unsigned      // Invalidation Granularity
            {
                GLOBAL  = 1,                // Global
                DOMAIN  = 2,                // Domain-Selective
                PAGE    = 3,                // Page-Selective within Domain
            };

            Inv_dsc_tlb() : Inv_dsc (Type::TLB, std::to_underlying (Gran::GLOBAL) << 4) {}

            Inv_dsc_tlb (uint16 did) : Inv_dsc (Type::TLB, static_cast<uint64>(did) << 16 | std::to_underlying (Gran::DOMAIN) << 4) {}
        };

        static_assert (__is_standard_layout (Inv_dsc_tlb) && sizeof (Inv_dsc_tlb) == sizeof (Inv_dsc));

        /*
         * IEC Invalidation Descriptor
         */
        struct Inv_dsc_iec final : Inv_dsc
        {
            enum class Gran : unsigned      // Invalidation Granularity
            {
                GLOBAL  = 0,                // Global
                INDEX   = 1,                // Index-Selective
            };

            Inv_dsc_iec() : Inv_dsc (Type::IEC, std::to_underlying (Gran::GLOBAL) << 4) {}

            Inv_dsc_iec (uint16 idx) : Inv_dsc (Type::IEC, static_cast<uint64>(idx) << 32 | std::to_underlying (Gran::INDEX) << 4) {}
        };

        static_assert (__is_standard_layout (Inv_dsc_iec) && sizeof (Inv_dsc_iec) == sizeof (Inv_dsc));

        Paddr const         phys_base;
        uintptr_t const     mmio_base;
        uint64              cap;
        uint64              ecap;
        unsigned            invq_idx            { 0 };
        Inv_dsc *           invq                { nullptr };
        Spinlock            inv_lock;

        static        Slab_cache    cache;
        static inline uintptr_t     mmap        { MMAP_GLB_SMMU };
        static inline Smmu *        list        { nullptr };
        static inline Smmu_ctx *    ctx         { new Smmu_ctx };
        static inline Smmu_irt *    irt         { new Smmu_irt };
        static inline Spinlock      cfg_lock;

        static constexpr unsigned ord { 0 };
        static constexpr unsigned cnt { (PAGE_SIZE << ord) / sizeof (Inv_dsc) };

        inline bool has_co() const { return ecap & BIT (0); }
        inline bool has_qi() const { return ecap & BIT (1); }
        inline bool has_dt() const { return ecap & BIT (2); }
        inline bool has_ir() const { return ecap & BIT (3); }

        inline auto nfr() const { return static_cast<unsigned>(cap >> 40 & BIT_RANGE (7, 0)) + 1; }
        inline auto fro() const { return static_cast<unsigned>(cap >> 20 & BIT_RANGE (13, 4)); }
        inline auto iro() const { return static_cast<unsigned>(ecap >> 4 & BIT_RANGE (13, 4)); }

        inline auto read  (Register32 r) const    { return *reinterpret_cast<uint32 volatile *>(mmio_base         + std::to_underlying (r)); }
        inline auto read  (Register64 r) const    { return *reinterpret_cast<uint64 volatile *>(mmio_base         + std::to_underlying (r)); }
        inline auto read  (Tlb64      r) const    { return *reinterpret_cast<uint64 volatile *>(mmio_base + iro() + std::to_underlying (r)); }

        inline void write (Register32 r, uint32 v) const { *reinterpret_cast<uint32 volatile *>(mmio_base         + std::to_underlying (r)) = v; }
        inline void write (Register64 r, uint64 v) const { *reinterpret_cast<uint64 volatile *>(mmio_base         + std::to_underlying (r)) = v; }
        inline void write (Tlb64      r, uint64 v) const { *reinterpret_cast<uint64 volatile *>(mmio_base + iro() + std::to_underlying (r)) = v; }

        ALWAYS_INLINE
        inline void read (unsigned frr, uint64 &hi, uint64 &lo) const
        {
            lo = *reinterpret_cast<uint64 volatile *>(mmio_base + fro() + frr * 16);
            hi = *reinterpret_cast<uint64 volatile *>(mmio_base + fro() + frr * 16 + 8);
            *reinterpret_cast<uint64 volatile *>(mmio_base + fro() + frr * 16 + 8) = BIT64 (63);
        }

        void command (Cmd c) const
        {
            // Mask one-shot bits
            auto const v { read (Register32::GSTS) & ~(Cmd::RTP | Cmd::FL | Cmd::WBF | Cmd::IRTP) };

            // Only one bit may be modified at a time
            write (Register32::GCMD, v | c);

            // Wait for hardware to signal completion
            while (!(read (Register32::GSTS) & c))
                pause();
        }

        /*
         * QI: Invalidation
         */
        void qi_post (Inv_dsc const &q)
        {
            invq[invq_idx] = q;
            invq_idx = (invq_idx + 1) % cnt;
            write (Register64::IQT, invq_idx << 4);
        };

        /*
         * QI: Wait for completion
         */
        void qi_wait() const { for (auto v { read (Register64::IQT) }; v != read (Register64::IQH); pause()) ; }

        /*
         * RI: Wait for TLB completion
         */
        void ri_wait_tlb() const { for (; read (Tlb64::IOTLB) & BIT64 (63); pause()) ; }

        /*
         * RI: Wait for CTX completion
         */
        void ri_wait_ctx() const { for (; read (Register64::CCMD) & BIT64 (63); pause()) ; }

        /*
         * RI: TLB Invalidation (Global)
         */
        void ri_inv_tlb() const
        {
            write (Tlb64::IOTLB, BIT64 (63) | static_cast<uint64>(Inv_dsc_tlb::Gran::GLOBAL) << 60);
            ri_wait_tlb();
        }

        /*
         * RI: TLB Invalidation (Domain-Selective)
         */
        void ri_inv_tlb (uint16 did) const
        {
            write (Tlb64::IOTLB, BIT64 (63) | static_cast<uint64>(Inv_dsc_tlb::Gran::DOMAIN) << 60 | static_cast<uint64>(did) << 32);
            ri_wait_tlb();
        }

        /*
         * RI: CTX Invalidation (Global)
         */
        void ri_inv_ctx() const
        {
            write (Register64::CCMD, BIT64 (63) | static_cast<uint64>(Inv_dsc_ctx::Gran::GLOBAL) << 61);
            ri_wait_ctx();
        }

        /*
         * RI: CTX Invalidation (Domain-Selective)
         */
        void ri_inv_ctx (uint16 did) const
        {
            write (Register64::CCMD, BIT64 (63) | static_cast<uint64>(Inv_dsc_ctx::Gran::DOMAIN) << 61 | static_cast<uint64>(did));
            ri_wait_ctx();
        }

        /*
         * RI: CTX Invalidation (Device-Selective)
         */
        void ri_inv_ctx (uint16 sid, uint16 did) const
        {
            write (Register64::CCMD, BIT64 (63) | static_cast<uint64>(Inv_dsc_ctx::Gran::DEVICE) << 61 | static_cast<uint64>(sid) << 16 | static_cast<uint64>(did));
            ri_wait_ctx();
        }

        /*
         * TLB Invalidation (Global)
         */
        void invalidate_tlb()
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_TRUE (has_qi())) {
                qi_post (Inv_dsc_tlb());
                qi_wait();
            } else
                ri_inv_tlb();
        }

        /*
         * TLB Invalidation (Domain-Selective)
         */
        void invalidate_tlb (uint16 did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_TRUE (has_qi())) {
                qi_post (Inv_dsc_tlb (did));
                qi_wait();
            } else
                ri_inv_tlb (did);
        }

        /*
         * CTX Invalidation (Global)
         */
        void invalidate_ctx()
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_TRUE (has_qi())) {
                qi_post (Inv_dsc_ctx());
                qi_post (Inv_dsc_tlb());
                qi_wait();
            } else {
                ri_inv_ctx();
                ri_inv_tlb();
            }
        }

        /*
         * CTX Invalidation (Domain-Selective)
         */
        void invalidate_ctx (uint16 did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_TRUE (has_qi())) {
                qi_post (Inv_dsc_ctx (did));
                qi_post (Inv_dsc_tlb (did));
                qi_wait();
            } else {
                ri_inv_ctx (did);
                ri_inv_tlb (did);
            }
        }

        /*
         * CTX Invalidation (Device-Selective)
         */
        void invalidate_ctx (uint16 sid, uint16 did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_TRUE (has_qi())) {
                qi_post (Inv_dsc_ctx (sid, did));
                qi_post (Inv_dsc_tlb (did));
                qi_wait();
            } else {
                ri_inv_ctx (sid, did);
                ri_inv_tlb (did);
            }
        }

        /*
         * IEC Invalidation (Global)
         */
        void invalidate_iec()
        {
            assert (ir && has_qi());    // IR support implies QI support

            Lock_guard <Spinlock> guard { inv_lock };

            qi_post (Inv_dsc_iec());
            qi_wait();
        }

        /*
         * IEC Invalidation (Index-Selective)
         */
        void invalidate_iec (uint16 idx)
        {
            assert (ir && has_qi());    // IR support implies QI support

            Lock_guard <Spinlock> guard { inv_lock };

            qi_post (Inv_dsc_iec (idx));
            qi_wait();
        }

        /*
         * Set Root Table Pointer
         */
        void set_rtp (uint64 addr)
        {
            write (Register64::RTADDR, addr);
            command (Cmd::RTP);

            if (!(cap & BIT64 (63)))
                invalidate_ctx();

            command (Cmd::TE);
        }

        /*
         * Set Interrupt Remapping Table Pointer
         */
        void set_irt (uint64 addr)
        {
            if (EXPECT_FALSE (!ir))
                return;

            write (Register64::IRTA, addr | 7);
            command (Cmd::IRTP);

            if (!(cap & BIT64 (62)))
                invalidate_iec();

            command (Cmd::IRE);
        }

        void fault();

        void init();

    public:
        static inline bool ir { false };

        explicit Smmu (Paddr);

        ALWAYS_INLINE
        static inline void init_all()
        {
            for (auto l { list }; l; l = l->next)
                l->init();
        }

        ALWAYS_INLINE
        static inline void invalidate_tlb_all (Sdid did)
        {
            for (auto l { list }; l; l = l->next)
                l->invalidate_tlb (did);
        }

        ALWAYS_INLINE
        static inline void interrupt()
        {
            for (auto l { list }; l; l = l->next)
                l->fault();
        }

        bool configure (Space_dma *, uintptr_t, bool = true);

        ALWAYS_INLINE
        static inline void set_irt (uint16 idx, uint16 rid, uint32 aid, uint8 vec, bool trg)
        {
            if (EXPECT_FALSE (!ir))
                return;

            irt[idx].set (BIT (18) | rid, static_cast<uint64>(aid) << 40 | vec << 16 | trg << 4 | BIT (0));

            for (auto l { list }; l; l = l->next)
                l->invalidate_iec (idx);
        }

        static inline Smmu *lookup (Paddr p)
        {
            for (auto l { list }; l; l = l->next)
                if (l->phys_base == p)
                    return l;

            return nullptr;
        }

        [[nodiscard]] ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }
};
