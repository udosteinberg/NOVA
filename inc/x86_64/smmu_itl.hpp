/*
 * System Memory Management Unit (Intel IOMMU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "smmu.hpp"

class Smmu_itl final : public Smmu
{
    friend class Interrupt;

    private:
        // Configurable Sizes
        static constexpr unsigned short ord_inv { 0 };  // 4K

        // Hardware Constraints
        static_assert (ord_inv <= 7, "INV queue must be <= 512K");

        enum class Reg32 : unsigned
        {
            VER         = 0x000,            // r-   Version
            GCMD        = 0x018,            // -w   Global Command
            GSTS        = 0x01c,            // r-   Global Status
            FSTS        = 0x034,            // rw   Fault Status
            FECTL       = 0x038,            // rw   Fault Event Control
            FEDATA      = 0x03c,            // rw   Fault Event Data
            FEADDR      = 0x040,            // rw   Fault Event Address
            FEUADDR     = 0x044,            // rw   Fault Event Upper Address
            PMEN        = 0x064,            // rw   Protected Memory Enable
        };

        enum class Reg64 : unsigned
        {
            CAP         = 0x008,            // r-   Capability
            ECAP        = 0x010,            // r-   Extended Capability
            RTADDR      = 0x020,            // rw   Root Table Address
            CCMD        = 0x028,            // rw   Context Command
            IQH         = 0x080,            // r-   Invalidation Queue Head
            IQT         = 0x088,            // rw   Invalidation Queue Tail
            IQA         = 0x090,            // rw   Invalidation Queue Address
            IQERCD      = 0x0b0,            // r-   Invalidation Queue Error Record
            IRTA        = 0x0b8,            // rw   Interrupt Remapping Table Address
        };

        enum class Tlb64 : unsigned
        {
            IVA         = 0x000,            // -w   Invalidate Address
            IOTLB       = 0x008,            // rw   IOTLB Invalidate
        };

        enum class Cap : uint64_t
        {
            ESRTPS      = BITN (63),        // r-   Enhanced Set Root Table Pointer Support
            ESIRTPS     = BITN (62),        // r-   Enhanced Set Interrupt Remap Table Pointer Support
            PHMR        = BITN  (6),        // r-   Protected Hi Memory Region
            PLMR        = BITN  (5),        // r-   Protected Lo Memory Region
            RWBF        = BITN  (4),        // r-   Required Write-Buffer Flushing
            AFL         = BITN  (3),        // r-   Advanced Fault Logging
        };

        enum class Ecap : uint64_t
        {
            EIM         = BITN  (4),        // r-   Extended Interrupt Mode
            IR          = BITN  (3),        // r-   Interrupt Remapping
            DT          = BITN  (2),        // r-   Device TLB
            QI          = BITN  (1),        // r-   Queued Invalidation
            PWC         = BITN  (0),        // r-   Page Walk Coherency
        };

        enum Cmd
        {
            TE          = BIT (31),         // -w   Translation Enable
            SRTP        = BIT (30),         // -w   Set Root Table Pointer
            SFL         = BIT (29),         // -w   Set Fault Log                           (only if Cap::AFL)
            EAFL        = BIT (28),         // -w   Advanced Fault Logging                  (only if Cap::AFL)
            WBF         = BIT (27),         // -w   Write Buffer Flush                      (only if Cap::RWBF)
            QIE         = BIT (26),         // -w   Queued Invalidation Enable              (only if Ecap::QI)
            IRE         = BIT (25),         // -w   Interrupt Remapping Enable              (only if Ecap::IR)
            SIRTP       = BIT (24),         // -w   Set Interrupt Remapping Table Pointer   (only if Ecap::IR)
            CFI         = BIT (23),         // -w   Compatibility Format Interrupt          (only if Ecap::IR)
        };

        enum Fault
        {
            ITE         = BIT (6),          // rw1c Invalidation Timeout Error              (only if Ecap::DT)
            ICE         = BIT (5),          // rw1c Invalidation Completion Error           (only if Ecap::DT)
            IQE         = BIT (4),          // rw1c Invalidation Queue Error                (only if Ecap::QI)
            APF         = BIT (3),          // rw1c Advanced Pending Fault                  (only if Cap::AFL)
            AFO         = BIT (2),          // rw1c Advanced Fault Overflow                 (only if Cap::AFL)
            PPF         = BIT (1),          // r-   Primary Pending Fault
            PFO         = BIT (0),          // rw1c Primary Fault Overflow
        };

        /*
         * Invalidation Descriptor
         */
        class Inv : public Queue::Entry
        {
            protected:
                enum class Type : unsigned  // Invalidation Type
                {
                    CTX = 1,                // Context-Cache
                    TLB = 2,                // IOTLB
                    DEV = 3,                // Device TLB
                    IEC = 4,                // Interrupt Entry Cache
                    IWT = 5,                // Invalidation Wait
                };

                explicit constexpr Inv (Type t, uint64_t l = 0, uint64_t h = 0) : Queue::Entry { (std::to_underlying (t) & BIT_RANGE (6, 4)) << 5 | (std::to_underlying (t) & BIT_RANGE (3, 0)) | l, h } {}
        };

        static_assert (__is_standard_layout (Inv) && alignof (Inv) == 16 && sizeof (Inv) == 16);

        /*
         * Section 6.5.2.1: Invalidation Descriptor: Context-Cache
         */
        struct Inv_ctx : Inv
        {
            enum class Gran : uint64_t      // Invalidation Granularity
            {
                GLB = 1,                    // Global
                DOM = 2,                    // Domain-Selective
                DEV = 3,                    // Device-Selective
            };

            explicit constexpr Inv_ctx (Gran g, uint64_t l = 0, uint64_t h = 0) : Inv { Type::CTX, (std::to_underlying (g) & BIT_RANGE (1, 0)) << 4 | l, h } {}
        };

        static_assert (__is_standard_layout (Inv_ctx) && alignof (Inv_ctx) == alignof (Inv) && sizeof (Inv_ctx) == sizeof (Inv));

        struct Inv_ctx_glb final : Inv_ctx { explicit constexpr Inv_ctx_glb()                            : Inv_ctx { Gran::GLB } {} };
        struct Inv_ctx_dom final : Inv_ctx { explicit constexpr Inv_ctx_dom (uint16_t dom)               : Inv_ctx { Gran::DOM, uint32_t { dom } << 16 } {} };
        struct Inv_ctx_dev final : Inv_ctx { explicit constexpr Inv_ctx_dev (uint16_t dom, uint16_t bdf) : Inv_ctx { Gran::DEV, uint32_t { dom } << 16 | uint64_t { bdf } << 32 } {} };

        /*
         * Section 6.5.2.3: Invalidation Descriptor: IOTLB
         */
        struct Inv_tlb : Inv
        {
            enum class Gran : uint64_t      // Invalidation Granularity
            {
                GLB = 1,                    // Global
                DOM = 2,                    // Domain-Selective
                PWD = 3,                    // Page-Selective within Domain
            };

            explicit constexpr Inv_tlb (Gran g, uint64_t l = 0, uint64_t h = 0) : Inv { Type::TLB, (std::to_underlying (g) & BIT_RANGE (1, 0)) << 4 | l, h } {}
        };

        static_assert (__is_standard_layout (Inv_tlb) && alignof (Inv_tlb) == alignof (Inv) && sizeof (Inv_tlb) == sizeof (Inv));

        struct Inv_tlb_glb final : Inv_tlb { explicit constexpr Inv_tlb_glb()                            : Inv_tlb { Gran::GLB } {} };
        struct Inv_tlb_dom final : Inv_tlb { explicit constexpr Inv_tlb_dom (uint16_t dom)               : Inv_tlb { Gran::DOM, uint32_t { dom } << 16 } {} };

        /*
         * Section 6.5.2.8: Invalidation Descriptor: Interrupt Entry Cache
         */
        struct Inv_iec : Inv
        {
            enum class Gran : uint64_t      // Invalidation Granularity
            {
                GLB = 0,                    // Global
                IDX = 1,                    // Index-Selective
            };

            explicit constexpr Inv_iec (Gran g, uint64_t l = 0, uint64_t h = 0) : Inv { Type::IEC, (std::to_underlying (g) & BIT_RANGE (1, 0)) << 4 | l, h } {}
        };

        static_assert (__is_standard_layout (Inv_iec) && alignof (Inv_iec) == alignof (Inv) && sizeof (Inv_iec) == sizeof (Inv));

        struct Inv_iec_glb final : Inv_iec { explicit constexpr Inv_iec_glb()                            : Inv_iec { Gran::GLB } {} };
        struct Inv_iec_idx final : Inv_iec { explicit constexpr Inv_iec_idx (uint16_t idx)               : Inv_iec { Gran::IDX, uint64_t { idx } << 32 } {} };

        /*
         * Section 6.5.2.9: Invalidation Descriptor: Invalidation Wait
         */
        struct Inv_iwt final : Inv
        {
            explicit constexpr Inv_iwt (uint32_t volatile &var, uint32_t data) : Inv { Type::IWT, uint64_t { data } << 32 | BIT (5), Cpu::loc_to_phys (&var) } {}
        };

        static_assert (__is_standard_layout (Inv_iwt) && alignof (Inv_iwt) == alignof (Inv) && sizeof (Inv_iwt) == sizeof (Inv));

        /*
         * Device Table
         */
        class Devtable final
        {
            public:
                class Entry
                {
                    protected:
                        uint128_t val;

                    public:
                        [[nodiscard]] bool present() const { return val & BIT (0); }

                        explicit constexpr Entry (uint128_t v = 0) : val { v } {}
                };

                static_assert (__is_standard_layout (Entry) && alignof (Entry) == 16 && sizeof (Entry) == 16);

                /*
                 * Section 9.1: Root Entry
                 */
                struct Rte final : public Entry
                {
                    [[nodiscard]] auto table() const { return static_cast<Devtable *>(Kmem::phys_to_ptr (static_cast<uint64_t>(val) & ~OFFS_MASK (0))); }

                    [[nodiscard]] auto update (Rte &o, Rte n)
                    {
                        // Attempt RTE update
                        if (!Atomic128::compare_exchange (val, o.val, n.val)) [[unlikely]]
                            return false;

                        // Make RTE update visible to non-coherent observers
                        if (true) [[likely]]
                            Cache::data_clean (this);

                        return true;
                    }

                    explicit Rte (Devtable *ctx) : Entry { Kmem::ptr_to_phys (ctx) | BIT (0) } {}
                };

                static_assert (__is_standard_layout (Rte) && alignof (Rte) == alignof (Entry) && sizeof (Rte) == sizeof (Entry));

                /*
                 * Section 9.3: Context Entry
                 */
                struct Cte final : public Entry
                {
                    [[nodiscard]] auto dom() const { return static_cast<uint16_t>(val >> 72); }

                    [[nodiscard]] auto update (Cte n)
                    {
                        Cte o;

                        // Perform DTE update
                        Atomic128::exchange (val, o.val, n.val);

                        // Make CTE update visible to non-coherent observers
                        if (true) [[likely]]
                            Cache::data_clean (this);

                        return o;
                    }

                    explicit constexpr Cte() = default;

                    explicit Cte (void *ptr, unsigned ptl, uint16_t dom) : Entry { uint128_t { dom } << 72 | uint128_t { ptl - 2 } << 64 | Kmem::ptr_to_phys (ptr) | BIT (0) } {}
                };

                static_assert (__is_standard_layout (Cte) && alignof (Cte) == alignof (Entry) && sizeof (Cte) == sizeof (Entry));

                [[nodiscard]] Cte *entry (pci_t src)
                {
                    // Determine RTE pointer
                    Rte *ptr { static_cast<Rte *>(&slot[Pci::bus (src)]) }, rte { *ptr };

                    // Install context table if missing
                    if (!rte.present()) [[unlikely]] {

                        // Allocate empty CTX table
                        auto const ctx { new Devtable };
                        if (!ctx) [[unlikely]]
                            return nullptr;

                        // Construct RTE that refers to the new CTX table
                        Rte const tmp { ctx };

                        // Try to install the RTE
                        if (ptr->update (rte, tmp)) [[likely]]
                            rte = tmp;
                        else
                            delete ctx;
                    }

                    // RTE must be present now
                    assert (rte.present());

                    // Determine CTE pointer
                    return static_cast<Cte *>(&rte.table()->slot[Pci::ari (src)]);
                }

                [[nodiscard]] auto base() const { return Kmem::ptr_to_phys (this); }

                // Constructor
                explicit Devtable()
                {
                    // Make new Devtable visible to non-coherent observers
                    if (true) [[likely]]
                        Cache::data_clean (this, sizeof (*this));
                }

                // Allocator
                [[nodiscard]] static void *operator new (size_t) noexcept { return Buddy::allocator.alloc (0); }

                // Deallocator
                static void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<uintptr_t>(ptr)); }

            private:
                // Device Table Entries
                Entry slot[PAGE_SIZE (0) / sizeof (Entry)];

                static_assert (sizeof (slot) / sizeof (*slot) == 256);
        };

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

                [[nodiscard]] bool lookup (uint8_t &vec) const
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
                    auto const ptr { Buddy::allocator.alloc (order_p, Buddy::Fill::FILL_0) };

                    // FIXME: Not needed when all SMMUs are coherent
                    if (ptr) [[likely]]
                        Cache::data_clean (ptr, PAGE_SIZE (0) << order_p);

                    return ptr;
                }
        };

        static_assert (__is_standard_layout (Entry_irt) && alignof (Entry_irt) == 16 && sizeof (Entry_irt) == 16);

        uint64_t      const cap;
        uint64_t      const ecap;
        Devtable *    const dtbl;
        Entry_irt *   const irt;
        Queue               invq;

        // CPU-Local Completion Infrastructure
        static uint32_t sequence CPULOCAL;  // Sequence Number
        static uint32_t doorbell CPULOCAL;  // Doorbell

        [[nodiscard]] bool feature (Cap  c) const { return cap  & std::to_underlying (c); }
        [[nodiscard]] bool feature (Ecap e) const { return ecap & std::to_underlying (e); }

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

        unsigned invq_head() const { return read (Reg64::IQH) & BIT_RANGE (18, 4); }

        void clear_faults() const
        {
            write (Reg32::FSTS, feature (Ecap::DT) * (Fault::ITE | Fault::ICE) | feature (Ecap::QI) * Fault::IQE | feature (Cap::AFL) * (Fault::APF | Fault::AFO) | Fault::PFO);
        }

        [[nodiscard]] bool read_frr (unsigned n, uint128_t &val) const
        {
            // Determine FRR offset in MMIO space
            auto const o { mmio + fro() + n * 16 };

            auto const h { *reinterpret_cast<uint64_t volatile *>(o + 8) };

            // Valid only if F-bit is set
            if (!(h & BITN (63)))
                return false;

            auto const l { *reinterpret_cast<uint64_t volatile *>(o) };

            // Assemble full 128-bit value
            val = uint128_t { h } << 64 | l;

            // Clear F-bit
            *reinterpret_cast<uint64_t volatile *>(o + 8) = BITN (63);

            return true;
        }

        [[nodiscard]] bool command (Cmd c) const
        {
            // Mask one-shot bits
            auto const v { read (Reg32::GSTS) & ~(Cmd::SRTP | Cmd::SFL | Cmd::WBF | Cmd::SIRTP) };

            // Completion polarity of the one-shot bit
            bool const p { c != Cmd::WBF };

            // Only one bit may be modified at a time
            write (Reg32::GCMD, v | c);

            // Wait until hardware sets the status bit
            return Wait::until (timeout, [&] { return !!(read (Reg32::GSTS) & c) == p; });
        }

        [[nodiscard]] inline auto invq_submit (auto const &...i)
        {
            // Build a batch of inv pointers from the fold expression
            Inv const *inv[] { &i... };

            // Submit the entire batch
            return invq_submit (inv, sizeof...(i));
        }

        [[nodiscard]] bool invq_submit (Inv const **i, size_t n)
        {
            auto const complete { ++sequence };

            {   Lock_guard <Spinlock> guard { invq.lock };

                // Wait until the queue has sufficient (n+1) capacity
                if (!Wait::until (timeout, [&] { return invq.num (invq_head(), true) > n; })) [[unlikely]]
                    return false;

                // Produce n invalidations
                while (n--)
                    invq.produce (**i++);

                // Append INVALIDATION_WAIT
                invq.produce (Inv_iwt { doorbell, complete });

                // Enforce ordering between queue writes (WB) and MMIO write (UC)
                Barrier::wmb();

                // Notify IOMMU
                write (Reg64::IQT, invq.offs());
            }

            // Wait for completion
            return Wait::doorbell (timeout, doorbell, complete);
        }

        /*
         * RI: Submit CTX command and wait for completion
         */
        [[nodiscard]] bool ri_ctx_submit (Inv_ctx::Gran g, uint64_t v = 0) const
        {
            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the CTX cache
            write (Reg64::CCMD, BITN (63) | (std::to_underlying (g) & BIT_RANGE (1, 0)) << 61 | v);

            return Wait::until (timeout, [&] { return !(read (Reg64::CCMD) & BITN (63)); });
        }

        /*
         * RI: Submit TLB command and wait for completion
         */
        [[nodiscard]] bool ri_tlb_submit (Inv_tlb::Gran g, uint64_t v = 0) const
        {
            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the IOTLB
            write (Tlb64::IOTLB, BITN (63) | (std::to_underlying (g) & BIT_RANGE (1, 0)) << 60 | (v & BITN_RANGE (49, 32)) | (read (Tlb64::IOTLB) & BITN_RANGE (31, 0)));

            return Wait::until (timeout, [&] { return !(read (Tlb64::IOTLB) & BITN (63)); });
        }

        /*
         * RI: CTX Invalidation (Global)
         */
        [[nodiscard]] bool ri_inv_ctx() const
        {
            return ri_ctx_submit (Inv_ctx::Gran::GLB);
        }

        /*
         * RI: CTX Invalidation (Domain-Selective)
         */
        [[nodiscard]] bool ri_inv_ctx (uint16_t dom) const
        {
            return ri_ctx_submit (Inv_ctx::Gran::DOM, dom);
        }

        /*
         * RI: CTX Invalidation (Device-Selective)
         */
        [[nodiscard]] bool ri_inv_ctx (uint16_t dom, uint16_t bdf) const
        {
            return ri_ctx_submit (Inv_ctx::Gran::DEV, uint32_t { bdf } << 16 | dom);
        }

        /*
         * RI: TLB Invalidation (Global)
         */
        [[nodiscard]] bool ri_inv_tlb() const
        {
            return ri_tlb_submit (Inv_tlb::Gran::GLB);
        }

        /*
         * RI: TLB Invalidation (Domain-Selective)
         */
        [[nodiscard]] bool ri_inv_tlb (uint16_t dom) const
        {
            return ri_tlb_submit (Inv_tlb::Gran::DOM, uint64_t { dom } << 32);
        }

        /*
         * Invalidation: CTX (Global)
         */
        [[nodiscard]] bool invalidate_dev()
        {
            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the CTX cache
            if (feature (Ecap::QI)) [[likely]]
                return invq_submit (Inv_ctx_glb {}, Inv_tlb_glb {});

            Lock_guard <Spinlock> guard { invq.lock };

            return ri_inv_ctx() && ri_inv_tlb();
        }

        /*
         * Invalidation: CTX (Domain-Selective)
         */
        [[nodiscard]] bool invalidate_dev (uint16_t dom)
        {
            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the CTX cache
            if (feature (Ecap::QI)) [[likely]]
                return invq_submit (Inv_ctx_dom { dom }, Inv_tlb_dom { dom });

            Lock_guard <Spinlock> guard { invq.lock };

            return ri_inv_ctx (dom) && ri_inv_tlb (dom);
        }

        /*
         * Invalidation: CTX (Device-Selective)
         */
        [[nodiscard]] bool invalidate_dev (uint16_t dom, uint16_t bdf) override final
        {
            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the CTX cache
            if (feature (Ecap::QI)) [[likely]]
                return invq_submit (Inv_ctx_dev { dom, bdf }, Inv_tlb_dom { dom });

            Lock_guard <Spinlock> guard { invq.lock };

            return ri_inv_ctx (dom, bdf) && ri_inv_tlb (dom);
        }

        /*
         * Invalidation: TLB (Global)
         */
        [[nodiscard]] bool invalidate_tlb()
        {
            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the IOTLB
            if (feature (Ecap::QI)) [[likely]]
                return invq_submit (Inv_tlb_glb {});

            Lock_guard <Spinlock> guard { invq.lock };

            return ri_inv_tlb();
        }

        /*
         * Invalidation: TLB (Domain-Selective)
         */
        [[nodiscard]] bool invalidate_tlb (uint16_t dom) override final
        {
            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the IOTLB
            if (feature (Ecap::QI)) [[likely]]
                return invq_submit (Inv_tlb_dom { dom });

            Lock_guard <Spinlock> guard { invq.lock };

            return ri_inv_tlb (dom);
        }

        /*
         * Invalidation: INT (Global)
         */
        [[nodiscard]] bool invalidate_int()
        {
            // IR support implies QI support
            assert (ir && feature (Ecap::QI));

            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the IEC
            return invq_submit (Inv_iec_glb {});
        }

        /*
         * Invalidation: INT (Index-Selective)
         */
        [[nodiscard]] bool invalidate_int (uint16_t idx) override final
        {
            // IR support implies QI support
            assert (ir && feature (Ecap::QI));

            // If RWBF=1 HW must implicitly perform a write buffer flush before invalidating the IEC
            return invq_submit (Inv_iec_idx { idx });
        }

        /*
         * Initialize Invalidation Queue
         */
        [[nodiscard]] bool init_inv()
        {
            if (!feature (Ecap::QI)) [[unlikely]]
                return true;

            // Check allocation
            assert (invq.ptr);

            // Set invalidation queue address + descriptor width + queue size
            write (Reg64::IQA, invq.base() | (sizeof (Inv) == 32) * BIT (11) | ord_inv);
            write (Reg64::IQT, invq.offs());

            // Enable Queued Invalidation
            return command (Cmd::QIE);
        }

        /*
         * Initialize DEV Table
         */
        [[nodiscard]] bool init_dev()
        {
            // Check allocation
            assert (dtbl);

            // Set root table address + legacy mode
            write (Reg64::RTADDR, dtbl->base());

            // Activate
            if (!command (Cmd::SRTP)) [[unlikely]]
                return false;

            // Invalidate stale cached entries
            if (!feature (Cap::ESRTPS) && !invalidate_dev()) [[unlikely]]
                return false;

            // Enable DMA remapping
            return command (Cmd::TE);
        }

        /*
         * Initialize Interrupt Remapping Table
         */
        [[nodiscard]] bool init_irt()
        {
            // Check allocation
            assert (irt);

            if (!ir) [[unlikely]]
                return true;

            // Set interrupt remapping table address + interrupt mode + table size
            write (Reg64::IRTA, Kmem::ptr_to_phys (irt) | Lapic::x2apic << 11 | (Entry_irt::order_e - 1));

            // Activate
            if (!command (Cmd::SIRTP)) [[unlikely]]
                return false;

            // Invalidate stale cached entries
            if (!feature (Cap::ESIRTPS) && !invalidate_int()) [[unlikely]]
                return false;

            // Enable interrupt remapping
            return command (Cmd::IRE);
        }

        /*
         * Initialize Protected Memory Regions
         */
        [[nodiscard]] bool init_pmr() const
        {
            // Check if PMR are supported
            if (!feature (Cap::PLMR) && !feature (Cap::PHMR)) [[unlikely]]
                return true;

            // Disable PMR once DMA remapping is enabled and maintain RsvdP bits
            write (Reg32::PMEN, read (Reg32::PMEN) & ~BIT (31));

            return Wait::until (timeout, [&] { return !(read (Reg32::PMEN) & BIT (0)); });
        }

        void interrupt() override final;

        [[nodiscard]] bool init() override final;

        explicit Smmu_itl (uint64_t, pci_t, Devtable *, Entry_irt *);

        static Slab_cache cache;

        [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }

    public:
        static inline constinit bool ir { false };

        [[nodiscard]] static Smmu_itl *create (uint64_t, pci_t);

        static Entry_irt *lookup_irt (iid_t iid)
        {
            auto const seg { Intid::to_seg (iid) };
            auto const gsi { Intid::to_gsi (iid) };

            // Using unsigned { gsi } to prevent compiler warning about a tautological compare of a uint16_t against 2^16

            auto const smmu { static_cast<Smmu_itl *>(lookup_seg (seg)) };
            if (smmu && unsigned { gsi } < BIT (Smmu_itl::Entry_irt::order_e)) [[likely]]
                return smmu->irt + gsi;

            return nullptr;
        }

        [[nodiscard]] Status assign_dev (Pd *, uintptr_t, bool = true) override final;

        [[nodiscard]] static Status assign_int (Entry_irt *, iid_t, cpu_t, uint8_t, pci_t, uint8_t, uintptr_t &, uintptr_t &);
};
