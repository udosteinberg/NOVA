/*
 * System Memory Management Unit (Intel IOMMU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "cache.hpp"
#include "intid.hpp"
#include "lock_guard.hpp"
#include "mmio.hpp"
#include "pci.hpp"
#include "sdid.hpp"
#include "wait.hpp"

class Smmu final : public List<Smmu>, private Mmio
{
    friend class Interrupt;

    private:
        enum class Reg32 : unsigned
        {
            VER         = 0x000,            // r- Version
            GCMD        = 0x018,            // -w Global Command
            GSTS        = 0x01c,            // r- Global Status
            FSTS        = 0x034,            // rw Fault Status
            FECTL       = 0x038,            // rw Fault Event Control
            FEDATA      = 0x03c,            // rw Fault Event Data
            FEADDR      = 0x040,            // rw Fault Event Address
            FEUADDR     = 0x044,            // rw Fault Event Upper Address
            PMEN        = 0x064,            // rw Protected Memory Enable
        };

        enum class Reg64 : unsigned
        {
            CAP         = 0x008,            // r- Capability
            ECAP        = 0x010,            // r- Extended Capability
            RTADDR      = 0x020,            // rw Root Table Address
            CCMD        = 0x028,            // rw Context Command
            IQH         = 0x080,            // r- Invalidation Queue Head
            IQT         = 0x088,            // rw Invalidation Queue Tail
            IQA         = 0x090,            // rw Invalidation Queue Address
            IQERCD      = 0x0b0,            // r- Invalidation Queue Error Record
            IRTA        = 0x0b8,            // rw Interrupt Remapping Table Address
        };

        enum class Tlb64 : unsigned
        {
            IVA         = 0x000,            // -w Invalidate Address
            IOTLB       = 0x008,            // rw IOTLB Invalidate
        };

        enum class Cap : uint64_t
        {
            ESRTPS      = BIT64 (63),       // Enhanced Set Root Table Pointer Support
            ESIRTPS     = BIT64 (62),       // Enhanced Set Interrupt Remap Table Pointer Support
            PHMR        = BIT64  (6),       // Protected Hi Memory Region
            PLMR        = BIT64  (5),       // Protected Lo Memory Region
        };

        enum class Ecap : uint64_t
        {
            EIM         = BIT64  (4),       // Extended Interrupt Mode
            IR          = BIT64  (3),       // Interrupt Remapping
            QI          = BIT64  (1),       // Queued Invalidation
            PWC         = BIT64  (0),       // Page Walk Coherency
        };

        enum Cmd
        {
            TE          = BIT (31),         // Translation Enable
            SRTP        = BIT (30),         // Set Root Table Pointer
            FL          = BIT (29),         // Fault Log
            AFL         = BIT (28),         // Advanced Fault Logging
            WBF         = BIT (27),         // Write Buffer Flush
            QIE         = BIT (26),         // Queued Invalidation Enable
            IRE         = BIT (25),         // Interrupt Remapping Enable
            SIRTP       = BIT (24),         // Set Interrupt Remapping Table Pointer
            CFI         = BIT (23),         // Compatibility Format Interrupt
        };

        enum Fault
        {
            ITE         = BIT (6),          // Invalidation Timeout Error
            ICE         = BIT (5),          // Invalidation Completion Error
            IQE         = BIT (4),          // Invalidation Queue Error
            APF         = BIT (3),          // Advanced Pending Fault
            AFO         = BIT (2),          // Advanced Fault Overflow
            PPF         = BIT (1),          // Primary Pending Fault
            PFO         = BIT (0),          // Primary Fault Overflow
        };

        /*
         * Invalidation Descriptor
         */
        class Inv
        {
            private:
                uint128_t val;

            protected:
                enum class Type : unsigned  // Invalidation Type
                {
                    CTX = 1,                // Context-Cache
                    TLB = 2,                // IOTLB
                    DEV = 3,                // Device TLB
                    IEC = 4,                // Interrupt Entry Cache
                    IWT = 5,                // Invalidation Wait
                };

                explicit Inv (Type t, uint128_t v) : val { std::to_underlying (t) | v } {}
        };

        static_assert (__is_standard_layout (Inv) && alignof (Inv) == 16 && sizeof (Inv) == 16);

        /*
         * Invalidation Descriptor: Context-Cache
         */
        struct Inv_ctx : Inv
        {
            enum class Gran : unsigned      // Invalidation Granularity
            {
                GLB = 1,                    // Global
                DOM = 2,                    // Domain-Selective
                DEV = 3,                    // Device-Selective
            };

            explicit Inv_ctx (Gran g, uint128_t v = 0) : Inv { Type::CTX, std::to_underlying (g) << 4 | v } {}
        };

        static_assert (__is_standard_layout (Inv_ctx) && alignof (Inv_ctx) == alignof (Inv) && sizeof (Inv_ctx) == sizeof (Inv));

        struct Inv_ctx_glb final : Inv_ctx { explicit Inv_ctx_glb()                            : Inv_ctx { Gran::GLB } {} };
        struct Inv_ctx_dom final : Inv_ctx { explicit Inv_ctx_dom (uint16_t did)               : Inv_ctx { Gran::DOM, static_cast<uint32_t>(did) << 16 } {} };
        struct Inv_ctx_dev final : Inv_ctx { explicit Inv_ctx_dev (uint16_t did, uint16_t sid) : Inv_ctx { Gran::DEV, static_cast<uint32_t>(did) << 16 | static_cast<uint64_t>(sid) << 32 } {} };

        /*
         * Invalidation Descriptor: IOTLB
         */
        struct Inv_tlb : Inv
        {
            enum class Gran : unsigned      // Invalidation Granularity
            {
                GLB = 1,                    // Global
                DOM = 2,                    // Domain-Selective
                PWD = 3,                    // Page-Selective within Domain
            };

            explicit Inv_tlb (Gran g, uint128_t v = 0) : Inv { Type::TLB, std::to_underlying (g) << 4 | v } {}
        };

        static_assert (__is_standard_layout (Inv_tlb) && alignof (Inv_tlb) == alignof (Inv) && sizeof (Inv_tlb) == sizeof (Inv));

        struct Inv_tlb_glb final : Inv_tlb { explicit Inv_tlb_glb()                            : Inv_tlb { Gran::GLB } {} };
        struct Inv_tlb_dom final : Inv_tlb { explicit Inv_tlb_dom (uint16_t did)               : Inv_tlb { Gran::DOM, static_cast<uint32_t>(did) << 16 } {} };

        /*
         * Invalidation Descriptor: Interrupt Entry Cache
         */
        struct Inv_iec : Inv
        {
            enum class Gran : unsigned      // Invalidation Granularity
            {
                GLB = 0,                    // Global
                IDX = 1,                    // Index-Selective
            };

            explicit Inv_iec (Gran g, uint128_t v = 0) : Inv { Type::IEC, std::to_underlying (g) << 4 | v } {}
        };

        static_assert (__is_standard_layout (Inv_iec) && alignof (Inv_iec) == alignof (Inv) && sizeof (Inv_iec) == sizeof (Inv));

        struct Inv_iec_glb final : Inv_iec { explicit Inv_iec_glb()                            : Inv_iec { Gran::GLB } {} };
        struct Inv_iec_idx final : Inv_iec { explicit Inv_iec_idx (uint16_t idx)               : Inv_iec { Gran::IDX, static_cast<uint64_t>(idx) << 32 } {} };

        /*
         * Invalidation Descriptor: Invalidation Wait
         */
        struct Inv_iwt final : Inv
        {
            explicit Inv_iwt (uint64_t hpa, uint32_t sts) : Inv { Type::IWT, static_cast<uint128_t>(hpa) << 64 | static_cast<uint64_t>(sts) << 32 | BIT (5) } {}
        };

        static_assert (__is_standard_layout (Inv_iwt) && alignof (Inv_iwt) == alignof (Inv) && sizeof (Inv_iwt) == sizeof (Inv));

        /*
         * CTX Entry
         */
        class Entry_ctx final
        {
            private:
                uint128_t val;

            public:
                bool present() const { return val & BIT (0); }

                auto addr() const { return static_cast<uint64_t>(val) & ~OFFS_MASK (0); }

                auto did() const { return static_cast<uint16_t>(val >> 72); }

                bool set (uint64_t hi, uint64_t lo)
                {
                    // If "lock cmpxchg16b" fails, then some other CPU beat us to the update
                    if (!__sync_bool_compare_and_swap (&val, val, static_cast<uint128_t>(hi) << 64 | lo)) [[unlikely]]
                        return false;

                    // FIXME: Not needed when all SMMUs are coherent
                    Cache::data_clean (this);

                    return true;
                }

                [[nodiscard]] static void *operator new (size_t) noexcept
                {
                    auto const ptr { Buddy::alloc (0, Buddy::Fill::BITS0) };

                    // FIXME: We want to use Cache::data_clean (ptr, size) here, but per-CPU line size is not available yet
                    if (ptr) [[likely]]
                        for (auto p { static_cast<char const *>(ptr) }; p != static_cast<char const *>(ptr) + PAGE_SIZE (0); p += 32)
                            Cache::data_clean (p);

                    return ptr;
                }
        };

        static_assert (__is_standard_layout (Entry_ctx) && alignof (Entry_ctx) == 16 && sizeof (Entry_ctx) == 16);

        /*
         * IRT Entry
         */
        class Entry_irt final
        {
            private:
                uint128_t val;

            public:
                static constexpr unsigned order_e { 16 };               // Order: Table Entries (8...16)
                static constexpr unsigned order_p { order_e - 8 };      // Order: Pages Required (0...8)

                bool lookup (uint8_t &vec) const
                {
                    auto const v { __atomic_load_n (reinterpret_cast<uint32_t const *>(&val), __ATOMIC_ACQUIRE) };

                    // Obtain GSI vector
                    vec = static_cast<uint8_t>(v >> 16);

                    // Return GSI trigger (false = edge, true = level)
                    return v & BIT (4);
                }

                [[nodiscard]] bool set (uint64_t hi, uint64_t lo)
                {
                    // If "lock cmpxchg16b" fails, then some other CPU beat us to the update
                    if (!__sync_bool_compare_and_swap (&val, val, static_cast<uint128_t>(hi) << 64 | lo)) [[unlikely]]
                        return false;

                    // FIXME: Not needed when all SMMUs are coherent
                    Cache::data_clean (this);

                    return true;
                }

                [[nodiscard]] static void *operator new (size_t) noexcept
                {
                    auto const ptr { Buddy::alloc (order_p, Buddy::Fill::BITS0) };

                    // FIXME: We want to use Cache::data_clean (ptr, size) here, but per-CPU line size is not available yet
                    if (ptr) [[likely]]
                        for (auto p { static_cast<char const *>(ptr) }; p != static_cast<char const *>(ptr) + (PAGE_SIZE (0) << order_p); p += 32)
                            Cache::data_clean (p);

                    return ptr;
                }
        };

        static_assert (__is_standard_layout (Entry_irt) && alignof (Entry_irt) == 16 && sizeof (Entry_irt) == 16);

        /*
         * PCI Segment Group
         */
        class Grp final : public List<Grp>
        {
            private:
                static Slab_cache cache;

                static inline constinit Grp *list { nullptr };

                static Grp *lookup (uint16_t seg)
                {
                    for (auto l { list }; l; l = l->next)
                        if (l->seg == seg)
                            return l;

                    return nullptr;
                }

            public:
                uint16_t    const   seg;
                Entry_ctx * const   ctx;
                Entry_irt * const   irt;

                static Entry_ctx *lookup_ctx (pci_t pci)
                {
                    auto const seg { Pci::seg (pci) };
                    auto const bus { Pci::bus (pci) };
                    auto const grp { lookup (seg) };

                    if (grp) [[likely]]
                        return grp->ctx + bus;

                    return nullptr;
                }

                static Entry_irt *lookup_irt (iid_t iid)
                {
                    auto const seg { Intid::to_seg (iid) };
                    auto const gsi { Intid::to_gsi (iid) };
                    auto const grp { lookup (seg) };

                    if (grp && gsi < BIT (Smmu::Entry_irt::order_e)) [[likely]]
                        return grp->irt + gsi;

                    return nullptr;
                }

                static Grp *setup (uint16_t seg)
                {
                    auto const grp { lookup (seg) };

                    // Use existing segment group
                    if (grp) [[likely]]
                        return grp;

                    auto const ctx { new Entry_ctx };
                    auto const irt { new Entry_irt };

                    if (!ctx || !irt) [[unlikely]]
                        return nullptr;

                    // Allocate new segment group
                    return new Grp { seg, ctx, irt };
                }

                explicit Grp (uint16_t s, Entry_ctx *c, Entry_irt *i) : List { list }, seg { s }, ctx { c }, irt { i } {}

                [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }
        };

        uint64_t    const   cap;
        uint64_t    const   ecap;
        Grp       * const   grp;
        Inv       * const   inv;
        unsigned            inv_idx             { 0 };
        Spinlock            inv_lock;

        static Slab_cache   cache;

        static inline constinit Smmu *    list  { nullptr };
        static inline constinit Spinlock  cfg_lock;

        static constexpr unsigned ord { 0 };
        static constexpr unsigned cnt { (PAGE_SIZE (0) << ord) / sizeof (Inv) };
        static constexpr unsigned timeout { 10 };

        bool feature (Cap  c) const { return cap  & std::to_underlying (c); }
        bool feature (Ecap e) const { return ecap & std::to_underlying (e); }

        auto mll() const { return static_cast<unsigned>(1 + bit_scan_msb (cap >> 34 & BIT_RANGE (1, 0))); }
        auto lev() const { return static_cast<unsigned>(2 + bit_scan_msb (cap >>  8 & BIT_RANGE (4, 0))); }

        auto nfr() const { return static_cast<unsigned>(cap >> 40 & BIT_RANGE (7, 0)) + 1; }
        auto fro() const { return static_cast<unsigned>(cap >> 20 & BIT_RANGE (13, 4)); }
        auto iro() const { return static_cast<unsigned>(ecap >> 4 & BIT_RANGE (13, 4)); }

        auto read  (Reg32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmio         + std::to_underlying (r)); }
        auto read  (Reg64 r) const      { return *reinterpret_cast<uint64_t volatile *>(mmio         + std::to_underlying (r)); }
        auto read  (Tlb64 r) const      { return *reinterpret_cast<uint64_t volatile *>(mmio + iro() + std::to_underlying (r)); }

        void write (Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmio         + std::to_underlying (r)) = v; }
        void write (Reg64 r, uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio         + std::to_underlying (r)) = v; }
        void write (Tlb64 r, uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio + iro() + std::to_underlying (r)) = v; }

        void read (unsigned frr, uint64_t &hi, uint64_t &lo) const
        {
            lo = *reinterpret_cast<uint64_t volatile *>(mmio + fro() + frr * 16);
            hi = *reinterpret_cast<uint64_t volatile *>(mmio + fro() + frr * 16 + 8);
            *reinterpret_cast<uint64_t volatile *>(mmio + fro() + frr * 16 + 8) = BIT64 (63);
        }

        bool command (Cmd c) const
        {
            // Mask one-shot bits
            auto const v { read (Reg32::GSTS) & ~(Cmd::SRTP | Cmd::FL | Cmd::WBF | Cmd::SIRTP) };

            // Only one bit may be modified at a time
            write (Reg32::GCMD, v | c);

            // Wait until hardware sets the status bit
            return Wait::until (timeout, [&] { return read (Reg32::GSTS) & c; });
        }

        /*
         * QI: Invalidation
         */
        void qi_post (Inv const &i)
        {
            inv[inv_idx] = i;
            inv_idx = (inv_idx + 1) % cnt;
            write (Reg64::IQT, inv_idx << 4);
        }

        /*
         * QI: Wait for completion
         */
        [[nodiscard]] bool qi_wait() const
        {
            auto const v { read (Reg64::IQT) };

            return Wait::until (timeout, [&] { return read (Reg64::IQH) == v; });
        }

        /*
         * RI: Wait for TLB completion
         */
        [[nodiscard]] bool ri_wait_tlb() const
        {
            return Wait::until (timeout, [&] { return !(read (Tlb64::IOTLB) & BIT64 (63)); });
        }

        /*
         * RI: Wait for CTX completion
         */
        [[nodiscard]] bool ri_wait_ctx() const
        {
            return Wait::until (timeout, [&] { return !(read (Reg64::CCMD) & BIT64 (63)); });
        }

        /*
         * RI: TLB Invalidation (Global)
         */
        [[nodiscard]] bool ri_inv_tlb() const
        {
            write (Tlb64::IOTLB, BIT64 (63) | static_cast<uint64_t>(Inv_tlb::Gran::GLB) << 60);

            return ri_wait_tlb();
        }

        /*
         * RI: TLB Invalidation (Domain-Selective)
         */
        [[nodiscard]] bool ri_inv_tlb (uint16_t did) const
        {
            write (Tlb64::IOTLB, BIT64 (63) | static_cast<uint64_t>(Inv_tlb::Gran::DOM) << 60 | static_cast<uint64_t>(did) << 32);

            return ri_wait_tlb();
        }

        /*
         * RI: CTX Invalidation (Global)
         */
        [[nodiscard]] bool ri_inv_ctx() const
        {
            write (Reg64::CCMD, BIT64 (63) | static_cast<uint64_t>(Inv_ctx::Gran::GLB) << 61);

            return ri_wait_ctx();
        }

        /*
         * RI: CTX Invalidation (Domain-Selective)
         */
        [[nodiscard]] bool ri_inv_ctx (uint16_t did) const
        {
            write (Reg64::CCMD, BIT64 (63) | static_cast<uint64_t>(Inv_ctx::Gran::DOM) << 61 | static_cast<uint64_t>(did));

            return ri_wait_ctx();
        }

        /*
         * RI: CTX Invalidation (Device-Selective)
         */
        [[nodiscard]] bool ri_inv_ctx (uint16_t did, uint16_t sid) const
        {
            write (Reg64::CCMD, BIT64 (63) | static_cast<uint64_t>(Inv_ctx::Gran::DEV) << 61 | static_cast<uint64_t>(sid) << 16 | static_cast<uint64_t>(did));

            return ri_wait_ctx();
        }

        /*
         * TLB Invalidation (Global)
         */
        bool invalidate_tlb()
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (!feature (Ecap::QI)) [[unlikely]]
                return ri_inv_tlb();

            qi_post (Inv_tlb_glb {});

            return qi_wait();
        }

        /*
         * TLB Invalidation (Domain-Selective)
         */
        bool invalidate_tlb (uint16_t did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (!feature (Ecap::QI)) [[unlikely]]
                return ri_inv_tlb (did);

            qi_post (Inv_tlb_dom { did });

            return qi_wait();
        }

        /*
         * CTX Invalidation (Global)
         */
        bool invalidate_ctx()
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (!feature (Ecap::QI)) [[unlikely]]
                return ri_inv_ctx() && ri_inv_tlb();

            qi_post (Inv_ctx_glb {});
            qi_post (Inv_tlb_glb {});

            return qi_wait();
        }

        /*
         * CTX Invalidation (Domain-Selective)
         */
        bool invalidate_ctx (uint16_t did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (!feature (Ecap::QI)) [[unlikely]]
                return ri_inv_ctx (did) && ri_inv_tlb (did);

            qi_post (Inv_ctx_dom { did });
            qi_post (Inv_tlb_dom { did });

            return qi_wait();
        }

        /*
         * CTX Invalidation (Device-Selective)
         */
        bool invalidate_ctx (uint16_t sid, uint16_t did)
        {
            Lock_guard <Spinlock> guard { inv_lock };

            if (!feature (Ecap::QI)) [[unlikely]]
                return ri_inv_ctx (did, sid) && ri_inv_tlb (did);

            qi_post (Inv_ctx_dev { did, sid });
            qi_post (Inv_tlb_dom { did });

            return qi_wait();
        }

        /*
         * IEC Invalidation (Global)
         */
        bool invalidate_iec()
        {
            assert (ir && feature (Ecap::QI));  // IR support implies QI support

            Lock_guard <Spinlock> guard { inv_lock };

            qi_post (Inv_iec_glb {});

            return qi_wait();
        }

        /*
         * IEC Invalidation (Index-Selective)
         */
        bool invalidate_iec (uint16_t idx)
        {
            assert (ir && feature (Ecap::QI));  // IR support implies QI support

            Lock_guard <Spinlock> guard { inv_lock };

            qi_post (Inv_iec_idx { idx });

            return qi_wait();
        }

        /*
         * Initialize Invalidation Queue
         */
        void init_inv()
        {
            if (!feature (Ecap::QI)) [[unlikely]]
                return;

            // Check allocation
            assert (inv);

            // Set invalidation queue address + 128-bit descriptors + 256 entries
            write (Reg64::IQA, Kmem::ptr_to_phys (inv));
            write (Reg64::IQT, inv_idx = 0);

            // Enable Queued Invalidation
            command (Cmd::QIE);
        }

        /*
         * Initialize Context Table
         */
        void init_ctx()
        {
            // Check allocation
            assert (grp);

            // Set root table address + legacy mode
            write (Reg64::RTADDR, Kmem::ptr_to_phys (grp->ctx));

            // Make active
            command (Cmd::SRTP);

            // Invalidate stale cached entries
            if (!feature (Cap::ESRTPS)) [[unlikely]]
                invalidate_ctx();

            // Enable DMA remapping
            command (Cmd::TE);
        }

        /*
         * Initialize Interrupt Remapping Table
         */
        void init_irt()
        {
            // Check allocation
            assert (grp);

            if (!ir) [[unlikely]]
                return;

            // Set interrupt remapping table address + interrupt mode + table size
            write (Reg64::IRTA, Kmem::ptr_to_phys (grp->irt) | Lapic::x2apic << 11 | (Entry_irt::order_e - 1));

            // Make active
            command (Cmd::SIRTP);

            // Invalidate stale cached entries
            if (!feature (Cap::ESIRTPS)) [[unlikely]]
                invalidate_iec();

            // Enable interrupt remapping
            command (Cmd::IRE);
        }

        /*
         * Initialize Protected Memory Regions
         */
        void init_pmr() const
        {
            // Check if PMR are supported
            if (!feature (Cap::PLMR) && !feature (Cap::PHMR)) [[unlikely]]
                return;

            // Disable PMR once DMA remapping is enabled and maintain RsvdP bits
            write (Reg32::PMEN, read (Reg32::PMEN) & ~BIT (31));

            Wait::until (timeout, [&] { return !(read (Reg32::PMEN) & BIT (0)); });
        }

        void fault();

        void init();

        explicit Smmu (uint64_t, Grp *, Inv *);

    public:
        static inline constinit bool ir { false };

        [[nodiscard]] static Smmu *setup (uint64_t, uint16_t);

        static void init_all()
        {
            // We need an IRT for PCI segment group 0, even if SMMU is not in use
            Grp::setup (0);

            for (auto l { list }; l; l = l->next)
                l->init();
        }

        static void all_invalidate_tlb (Sdid did)
        {
            for (auto l { list }; l; l = l->next)
                l->invalidate_tlb (did);
        }

        static void all_invalidate_ctx (uint16_t seg, uint16_t sid, uint16_t did)
        {
            for (auto l { list }; l; l = l->next)
                if (l->grp->seg == seg)
                    l->invalidate_ctx (sid, did);
        }

        static void all_invalidate_iec (uint16_t seg, uint16_t idx)
        {
            for (auto l { list }; l; l = l->next)
                if (l->grp->seg == seg)
                    l->invalidate_iec (idx);
        }

        static void interrupt()
        {
            for (auto l { list }; l; l = l->next)
                l->fault();
        }

        bool configured (pci_t pci) const
        {
            auto ctx { grp->ctx };

            auto const r { ctx + Pci::bus (pci) };

            if (!r->present())
                return false;

            auto const c { static_cast<Entry_ctx *>(Kmem::phys_to_ptr (r->addr())) + Pci::ari (pci) };

            return c->present();
        }

        Status assign_dev (Space_dma *, uintptr_t, bool = true);

        [[nodiscard]] static Status assign_int (Entry_irt *, iid_t, cpu_t, uint8_t, pci_t, uint8_t, uintptr_t &, uintptr_t &);

        [[nodiscard]] static Smmu *lookup (uint64_t p)
        {
            for (auto l { list }; l; l = l->next)
                if (l->phys == p)
                    return l;

            return nullptr;
        }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }
};
