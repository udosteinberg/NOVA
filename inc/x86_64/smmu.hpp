/*
 * System Memory Management Unit (Intel IOMMU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "lapic.hpp"
#include "list.hpp"
#include "lock_guard.hpp"
#include "memory.hpp"
#include "sdid.hpp"
#include "slab.hpp"
#include "stdio.hpp"

class Space_dma;

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
                uint64_t lo, hi;

            protected:
                enum class Type : unsigned  // Invalidation Type
                {
                    CTX = 1,                // Context-Cache
                    TLB = 2,                // IOTLB
                    DEV = 3,                // Device TLB
                    IEC = 4,                // Interrupt Entry Cache
                    IWT = 5,                // Invalidation Wait
                };

            public:
                Inv_dsc (Type t, uint64_t l, uint64_t h = 0) : lo (l | std::to_underlying (t)), hi (h) {}
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

            Inv_dsc_ctx (uint16_t did) : Inv_dsc (Type::CTX, static_cast<uint64_t>(did) << 16 | std::to_underlying (Gran::DOMAIN) << 4) {}

            Inv_dsc_ctx (uint16_t sid, uint16_t did) : Inv_dsc (Type::CTX, static_cast<uint64_t>(sid) << 32 | static_cast<uint64_t>(did) << 16 | std::to_underlying (Gran::DEVICE) << 4) {}
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

            Inv_dsc_tlb (uint16_t did) : Inv_dsc (Type::TLB, static_cast<uint64_t>(did) << 16 | std::to_underlying (Gran::DOMAIN) << 4) {}
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

            Inv_dsc_iec (uint16_t idx) : Inv_dsc (Type::IEC, static_cast<uint64_t>(idx) << 32 | std::to_underlying (Gran::INDEX) << 4) {}
        };

        static_assert (__is_standard_layout (Inv_dsc_iec) && sizeof (Inv_dsc_iec) == sizeof (Inv_dsc));

        /*
         * Invalidation Wait Descriptor
         */
        struct Inv_dsc_iwt final : Inv_dsc
        {
            Inv_dsc_iwt (uint64_t hpa, uint32_t val) : Inv_dsc (Type::IWT, static_cast<uint64_t>(val) << 32 | BIT (5), hpa) {}
        };

        static_assert (__is_standard_layout (Inv_dsc_iwt) && sizeof (Inv_dsc_iwt) == sizeof (Inv_dsc));

        /*
         * CTX Entry
         */
        class Entry_ctx final
        {
            private:
                uint64_t lo, hi;

            public:
                bool present() const { return lo & BIT (0); }

                auto addr() const { return lo & ~OFFS_MASK (0); }

                auto did() const { return static_cast<uint16_t>(hi >> 8); }

                void set (uint64_t h, uint64_t l) { hi = h; lo = l; Cache::data_clean (this); }

                [[nodiscard]] static void *operator new (size_t) noexcept
                {
                    auto const ptr { Buddy::alloc (0, Buddy::Fill::BITS0) };

                    // FIXME: We want to use Cache::data_clean (ptr, PAGE_SIZE (0)) here, but per-CPU line size is not available yet
                    if (EXPECT_TRUE (ptr))
                        for (auto p { static_cast<char *>(ptr) }; p < static_cast<char *>(ptr) + PAGE_SIZE (0); p += 32)
                            Cache::data_clean (p);

                    return ptr;
                }
        };

        /*
         * IRT Entry
         */
        class Entry_irt final
        {
            private:
                uint64_t lo, hi;

            public:
                void set (uint64_t h, uint64_t l) { hi = h; lo = l; Cache::data_clean (this); }

                [[nodiscard]] static void *operator new (size_t) noexcept
                {
                    auto const ptr { Buddy::alloc (0, Buddy::Fill::BITS0) };

                    // FIXME: We want to use Cache::data_clean (ptr, PAGE_SIZE (0)) here, but per-CPU line size is not available yet
                    if (EXPECT_TRUE (ptr))
                        for (auto p { static_cast<char *>(ptr) }; p < static_cast<char *>(ptr) + PAGE_SIZE (0); p += 32)
                            Cache::data_clean (p);

                    return ptr;
                }
        };

        uint64_t const      phys_base;
        uintptr_t const     mmio_base;
        uint64_t            cap;
        uint64_t            ecap;
        unsigned            invq_idx            { 0 };
        Inv_dsc *           invq                { nullptr };
        Spinlock            inv_lock;

        static        Slab_cache    cache;
        static inline uintptr_t     mmap        { MMAP_GLB_SMMU };
        static inline Smmu *        list        { nullptr };
        static inline Entry_ctx *   ctx         { new Entry_ctx };
        static inline Entry_irt *   irt         { new Entry_irt };
        static inline Spinlock      cfg_lock;

        static constexpr unsigned ord { 0 };
        static constexpr unsigned cnt { (PAGE_SIZE (0) << ord) / sizeof (Inv_dsc) };

        bool has_co()  const { return ecap & BIT (0); }
        bool has_qi()  const { return ecap & BIT (1); }
        bool has_dt()  const { return ecap & BIT (2); }
        bool has_ir()  const { return ecap & BIT (3); }
        bool has_eim() const { return ecap & BIT (4); }

        auto nfr() const { return static_cast<unsigned>(cap >> 40 & BIT_RANGE (7, 0)) + 1; }
        auto fro() const { return static_cast<unsigned>(cap >> 20 & BIT_RANGE (13, 4)); }
        auto iro() const { return static_cast<unsigned>(ecap >> 4 & BIT_RANGE (13, 4)); }

        auto read  (Register32 r) const    { return *reinterpret_cast<uint32_t volatile *>(mmio_base         | std::to_underlying (r)); }
        auto read  (Register64 r) const    { return *reinterpret_cast<uint64_t volatile *>(mmio_base         | std::to_underlying (r)); }
        auto read  (Tlb64      r) const    { return *reinterpret_cast<uint64_t volatile *>(mmio_base + iro() + std::to_underlying (r)); }

        void write (Register32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmio_base         | std::to_underlying (r)) = v; }
        void write (Register64 r, uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio_base         | std::to_underlying (r)) = v; }
        void write (Tlb64      r, uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio_base + iro() + std::to_underlying (r)) = v; }

        // Relative timeout in ms converted to TSC ticks = freq_in_hz * ms / 1000
        static constexpr auto timeout (unsigned ms) { return 1000'000'000ULL * ms / 1000; }

        ALWAYS_INLINE
        inline void read (unsigned frr, uint64_t &hi, uint64_t &lo) const
        {
            lo = *reinterpret_cast<uint64_t volatile *>(mmio_base + fro() + frr * 16);
            hi = *reinterpret_cast<uint64_t volatile *>(mmio_base + fro() + frr * 16 + 8);
            *reinterpret_cast<uint64_t volatile *>(mmio_base + fro() + frr * 16 + 8) = BIT64 (63);
        }

        bool command (Cmd c) const
        {
            // Mask one-shot bits
            auto const v { read (Register32::GSTS) & ~(Cmd::RTP | Cmd::FL | Cmd::WBF | Cmd::IRTP) };

            // Only one bit may be modified at a time
            write (Register32::GCMD, v | c);

            // Wait for hardware to signal completion
            for (auto const t { timeout (10) }, b { __builtin_ia32_rdtsc() }; !(read (Register32::GSTS) & c); __builtin_ia32_pause())
                if (EXPECT_FALSE (__builtin_ia32_rdtsc() - b > t)) {
                    trace (TRACE_ERROR, "SMMU: %#010lx %s hang", phys_base, __func__);
                    return false;
                }

            return true;
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
        [[nodiscard]] bool qi_wait() const
        {
            auto const v { read (Register64::IQT) };

            for (auto const t { timeout (10) }, b { __builtin_ia32_rdtsc() }; read (Register64::IQH) != v; __builtin_ia32_pause())
                if (EXPECT_FALSE (__builtin_ia32_rdtsc() - b > t)) {
                    trace (TRACE_ERROR, "SMMU: %#010lx %s hang", phys_base, __func__);
                    return false;
                }

            return true;
        }

        /*
         * RI: Wait for TLB completion
         */
        [[nodiscard]] bool ri_wait_tlb() const
        {
            for (auto const t { timeout (10) }, b { __builtin_ia32_rdtsc() }; read (Tlb64::IOTLB) & BIT64 (63); __builtin_ia32_pause())
                if (EXPECT_FALSE (__builtin_ia32_rdtsc() - b > t)) {
                    trace (TRACE_ERROR, "SMMU: %#010lx %s hang", phys_base, __func__);
                    return false;
                }

            return true;
        }

        /*
         * RI: Wait for CTX completion
         */
        [[nodiscard]] bool ri_wait_ctx() const
        {
            for (auto const t { timeout (10) }, b { __builtin_ia32_rdtsc() }; read (Register64::CCMD) & BIT64 (63); __builtin_ia32_pause())
                if (EXPECT_FALSE (__builtin_ia32_rdtsc() - b > t)) {
                    trace (TRACE_ERROR, "SMMU: %#010lx %s hang", phys_base, __func__);
                    return false;
                }

            return true;
        }

        /*
         * RI: TLB Invalidation (Global)
         */
        [[nodiscard]] bool ri_inv_tlb() const
        {
            write (Tlb64::IOTLB, BIT64 (63) | static_cast<uint64_t>(Inv_dsc_tlb::Gran::GLOBAL) << 60);
            return ri_wait_tlb();
        }

        /*
         * RI: TLB Invalidation (Domain-Selective)
         */
        [[nodiscard]] bool ri_inv_tlb (uint16_t did) const
        {
            write (Tlb64::IOTLB, BIT64 (63) | static_cast<uint64_t>(Inv_dsc_tlb::Gran::DOMAIN) << 60 | static_cast<uint64_t>(did) << 32);
            return ri_wait_tlb();
        }

        /*
         * RI: CTX Invalidation (Global)
         */
        [[nodiscard]] bool ri_inv_ctx() const
        {
            write (Register64::CCMD, BIT64 (63) | static_cast<uint64_t>(Inv_dsc_ctx::Gran::GLOBAL) << 61);
            return ri_wait_ctx();
        }

        /*
         * RI: CTX Invalidation (Domain-Selective)
         */
        [[nodiscard]] bool ri_inv_ctx (uint16_t did) const
        {
            write (Register64::CCMD, BIT64 (63) | static_cast<uint64_t>(Inv_dsc_ctx::Gran::DOMAIN) << 61 | static_cast<uint64_t>(did));
            return ri_wait_ctx();
        }

        /*
         * RI: CTX Invalidation (Device-Selective)
         */
        [[nodiscard]] bool ri_inv_ctx (uint16_t sid, uint16_t did) const
        {
            write (Register64::CCMD, BIT64 (63) | static_cast<uint64_t>(Inv_dsc_ctx::Gran::DEVICE) << 61 | static_cast<uint64_t>(sid) << 16 | static_cast<uint64_t>(did));
            return ri_wait_ctx();
        }

        /*
         * TLB Invalidation (Global)
         */
        bool invalidate_tlb()
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_FALSE (!has_qi()))
                return ri_inv_tlb();

            qi_post (Inv_dsc_tlb());

            return qi_wait();
        }

        /*
         * TLB Invalidation (Domain-Selective)
         */
        bool invalidate_tlb (uint16_t did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_FALSE (!has_qi()))
                return ri_inv_tlb (did);

            qi_post (Inv_dsc_tlb (did));

            return qi_wait();
        }

        /*
         * CTX Invalidation (Global)
         */
        bool invalidate_ctx()
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_FALSE (!has_qi()))
                return ri_inv_ctx() && ri_inv_tlb();

            qi_post (Inv_dsc_ctx());
            qi_post (Inv_dsc_tlb());

            return qi_wait();
        }

        /*
         * CTX Invalidation (Domain-Selective)
         */
        bool invalidate_ctx (uint16_t did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_FALSE (!has_qi()))
                return ri_inv_ctx (did) && ri_inv_tlb (did);

            qi_post (Inv_dsc_ctx (did));
            qi_post (Inv_dsc_tlb (did));

            return qi_wait();
        }

        /*
         * CTX Invalidation (Device-Selective)
         */
        bool invalidate_ctx (uint16_t sid, uint16_t did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (EXPECT_FALSE (!has_qi()))
                return ri_inv_ctx (sid, did) && ri_inv_tlb (did);

            qi_post (Inv_dsc_ctx (sid, did));
            qi_post (Inv_dsc_tlb (did));

            return qi_wait();
        }

        /*
         * IEC Invalidation (Global)
         */
        bool invalidate_iec()
        {
            assert (ir && has_qi());    // IR support implies QI support

            Lock_guard <Spinlock> guard { inv_lock };

            qi_post (Inv_dsc_iec());

            return qi_wait();
        }

        /*
         * IEC Invalidation (Index-Selective)
         */
        bool invalidate_iec (uint16_t idx)
        {
            assert (ir && has_qi());    // IR support implies QI support

            Lock_guard <Spinlock> guard { inv_lock };

            qi_post (Inv_dsc_iec (idx));

            return qi_wait();
        }

        /*
         * Set Root Table Pointer
         */
        void set_rtp (uint64_t addr)
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
        void set_irt (uint64_t addr)
        {
            if (EXPECT_FALSE (!ir))
                return;

            write (Register64::IRTA, addr | Lapic::x2apic << 11 | 7);
            command (Cmd::IRTP);

            if (!(cap & BIT64 (62)))
                invalidate_iec();

            command (Cmd::IRE);
        }

        void fault();

        void init();

    public:
        static inline bool ir { false };

        explicit Smmu (uint64_t);

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

        bool configured (pci_t src) const
        {
            auto const r { ctx + (src >> 8) };

            if (!r->present())
                return false;

            auto const c { static_cast<Entry_ctx *>(Kmem::phys_to_ptr (r->addr())) + static_cast<uint8_t>(src) };

            return c->present();
        }

        static void set_irte (uint16_t idx, pci_t src, apic_t dst, uint8_t vec, bool trg)
        {
            if (EXPECT_FALSE (!ir))
                return;

            irt[idx].set (BIT (18) | src, static_cast<uint64_t>(dst) << (Lapic::x2apic ? 32 : 40) | vec << 16 | trg << 4 | BIT (0));

            for (auto l { list }; l; l = l->next)
                l->invalidate_iec (idx);
        }

        static inline Smmu *lookup (uint64_t p)
        {
            for (auto l { list }; l; l = l->next)
                if (l->phys_base == p)
                    return l;

            return nullptr;
        }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }
};
