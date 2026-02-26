/*
 * System Memory Management Unit (AMD IOMMU)
 *
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

#include "smmu.hpp"

class Smmu_amd final : public Smmu
{
    private:
        // Configurable Sizes
        static constexpr Buddy::order_t ord_dev { 9 };  // 2M
        static constexpr Buddy::order_t ord_cmd { 0 };  // 4K
        static constexpr Buddy::order_t ord_evt { 0 };  // 4K

        // Hardware Constraints
        static_assert (ord_dev <= 9, "DEV table must be <= 2M");
        static_assert (ord_cmd <= 7, "CMD queue must be <= 512K");
        static_assert (ord_evt <= 7, "EVT queue must be <= 512K");

        enum class Reg32 : unsigned
        {
            SDEV_MISC_0     = 0x0150,       // r-   Pci::Cap_sdev::Reg32::MISC_0
            SDEV_MISC_1     = 0x0154,       // rw   Pci::Cap_sdev::Reg32::MISC_1
            MSI_HDR         = 0x0158,       // rw   Pci::Cap_msi::Reg32::HDR
            MSI_F64_ADDR_LO = 0x015c,       // rw   Pci::Cap_msi::Reg32::F64_ADDR_LO
            MSI_F64_ADDR_HI = 0x0160,       // rw   Pci::Cap_msi::Reg32::F64_ADDR_HI
            MSI_F64_DATA    = 0x0164,       // rw   Pci::Cap_msi::Reg32::F64_DATA
            HT_HDR          = 0x0168,       // r-   Pci::Cap_ht::Reg32::HDR
            PERF_OPT_CTRL   = 0x016c,       // rw   Performance Optimization Control
        };

        enum class Reg64 : unsigned
        {
            DEV_TBL_BASE    = 0x0000,       // rw   Device Table Base Address
            CMD_BUF_BASE    = 0x0008,       // rw   Command Buffer Base Address
            EVT_LOG_BASE    = 0x0010,       // rw   Event Log Base Address (A)
            CTL             = 0x0018,       // rw   IOMMU Control
            EXC_RNG_BASE    = 0x0020,       // rw   Exclusion Range Base    / Completion Store Base
            EXC_RNG_LIMIT   = 0x0028,       // rw   Exclusion Range Limit   / Completion Store Limit
            EFR1            = 0x0030,       // r-   Extended Feature 1
            PPR_LOG_BASE    = 0x0038,       // rw   PPR Log Base Address (A)
            HW_ERR_UPPER    = 0x0040,       // rw   Hardware Event Upper
            HW_ERR_LOWER    = 0x0048,       // rw   Hardware Event Lower
            HW_ERR_STATUS   = 0x0050,       // rw   Hardware Event Status
            GVA_LOG_BASE    = 0x00e0,       // rw   Guest Virtual APIC Log Base Address
            GVA_LOG_TAIL    = 0x00e8,       // rw   Guest Virtual APIC Log Tail Address
            PPR_LOG_BASE_B  = 0x00f0,       // rw   PPR Log Base Address (B)
            EVT_LOG_BASE_B  = 0x00f8,       // rw   Event Log Base Address (B)
            DSFX            = 0x0138,       // r-   Device-Specific Feature Extension
            DSCX            = 0x0140,       // rw   Device-Specific Control Extension
            DSSX            = 0x0148,       // rw   Device-Specific Status Extension
            ICR_EVT         = 0x0170,       // rw   Interrupt Control Register (EVT)
            ICR_PPR         = 0x0178,       // rw   Interrupt Control Register (PPR)
            ICR_GVA         = 0x0180,       // rw   Interrupt Control Register (GVA)
            EFR2            = 0x01a0,       // r-   Extended Feature 2
            CMD_HEAD        = 0x2000,       // rw   Command Buffer Head Pointer
            CMD_TAIL        = 0x2008,       // rw   Command Buffer Tail Pointer
            EVT_HEAD        = 0x2010,       // rw   Event Log Head Pointer (A)
            EVT_TAIL        = 0x2018,       // rw   Event Log Tail Pointer (A)
            STS             = 0x2020,       // rw   IOMMU Status
            PPR_HEAD        = 0x2030,       // rw   PPR Log Head Pointer (A)
            PPR_TAIL        = 0x2038,       // rw   PPR Log Tail Pointer (A)
            GVA_HEAD        = 0x2040,       // rw   Guest Virtual APIC Log Head Pointer
            GVA_TAIL        = 0x2048,       // rw   Guest Virtual APIC Log Tail Pointer
            PPR_HEAD_B      = 0x2050,       // rw   PPR Log Head Pointer (B)
            PPR_TAIL_B      = 0x2058,       // rw   PPR Log Tail Pointer (B)
            EVT_HEAD_B      = 0x2070,       // rw   Event Log Head Pointer (B)
            EVT_TAIL_B      = 0x2078,       // rw   Event Log Tail Pointer (B)
        };

        enum CTL
        {
            E_XT_INT        = BITN (51),    // rw   Enable x2APIC interrupts
            E_XT            = BITN (50),    // rw   Enable x2APIC
            D_SELF_WB       = BITN (23),    // rw   Disable Self Writeback
            E_SMI_FILTER    = BITN (22),    // rw   Enable SMI Filter
            E_GA            = BITN (17),    // rw   Enable Guest Virtual APIC
            E_GT            = BITN (16),    // rw   Enable Guest Translation
            E_PPR           = BITN (15),    // rw   Enable PPR Processing
            E_INT_PPR       = BITN (14),    // rw   Enable Interrupt for PPR
            E_PPR_LOG       = BITN (13),    // rw   Enable PPR Log
            E_CMD_BUF       = BITN (12),    // rw   Enable Command Buffer
            E_ISOCHRONOUS   = BITN (11),    // rw   Enable Isochronous Bit in IOMMU Requests
            E_COHERENT      = BITN (10),    // rw   Enable Coherent Bit in IOMMU Requests
            E_INT_COM       = BITN  (4),    // rw   Enable Interrupt for Completions
            E_INT_EVT       = BITN  (3),    // rw   Enable Interrupt for Events
            E_EVT_LOG       = BITN  (2),    // rw   Enable Event Log
            E_HT_TUN        = BITN  (1),    // rw   Enable HyperTransport Tunnel Translation
            E_IOMMU         = BITN  (0),    // rw   Enable IOMMU
        };

        enum STS
        {
            PPR_EOW         = BITN (18),    // rw1c PPG Log A Early Overflow Warning
            PPR_EOW_B       = BITN (17),    // rw1c PPR Log B Early Overflow Warning
            EVT_ACT         = BITN (16),    // r-   Event Log B Active
            EVT_OVR_B       = BITN (15),    // rw1c Event Log B Overflow
            PPR_ACT         = BITN (12),    // r-   PPR Log B Active
            PPR_OVR_B       = BITN (11),    // rw1c PPR Log B Overflow
            GVA_INT         = BITN (10),    // rw1c Guest Virtual APIC Log Interrupt
            GVA_OVR         = BITN  (9),    // rw1c Guest Virtual APIC Log Overflow
            GVA_RUN         = BITN  (8),    // r-   Guest Virtual APIC Log Running
            PPR_RUN         = BITN  (7),    // r-   PPR Log Running
            PPR_INT         = BITN  (6),    // rw1c PPR Log Interrupt
            PPR_OVR         = BITN  (5),    // rw1c PPR Log Overflow
            CMD_RUN         = BITN  (4),    // r-   Command Buffer Running
            EVT_RUN         = BITN  (3),    // r-   Event Buffer Running
            COM_INT         = BITN  (2),    // rw1c Completion Interrupt
            EVT_INT         = BITN  (1),    // rw1c Event Log Interrupt
            EVT_OVR         = BITN  (0),    // rw1c Event Log Overflow
        };

        auto read  (Reg32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)); }
        auto read  (Reg64 r) const      { return *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r)); }
        void write (Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)) = v; }
        void write (Reg64 r, uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r)) = v; }

        unsigned cmdq_head() const { return read (Reg64::CMD_HEAD) & BIT_RANGE (18, 4); }
        unsigned evtq_tail() const { return read (Reg64::EVT_TAIL) & BIT_RANGE (18, 4); }

        /*
         * Device Table
         */
        class Devtable
        {
            private:
                class Entry
                {
                    private:
                        uint128_t lo {}, hi {};

                    public:
                        void set_lo (uint128_t &o, uint128_t n) { Atomic128::exchange (lo, o, n); }
                        void set_hi (uint128_t &o, uint128_t n) { Atomic128::exchange (hi, o, n); }
                };

                static_assert (__is_standard_layout (Entry) && alignof (Entry) == 16 && sizeof (Entry) == 32);

                // Device Table Entries
                Entry slot[PAGE_SIZE (0) / sizeof (Entry) << ord_dev];

            public:
                auto base() const { return Kmem::ptr_to_phys (this); }
                auto size() const { return sizeof (slot) / sizeof (*slot); }

                [[nodiscard]] auto set_dev (uint16_t bdf, uint16_t dom, void *ptr, unsigned ptl)
                {
                    uint128_t o;
                    slot[bdf].set_lo (o, uint128_t { dom } << 64 | BITN_RANGE (62, 61) | Kmem::ptr_to_phys (ptr) | ptl << 9 | BIT_RANGE (1, 0));
                    return o;
                }

                [[nodiscard]] static void *operator new (size_t) noexcept { return Buddy::alloc (ord_dev); }
        };

        static_assert (__is_standard_layout (Devtable) && alignof (Devtable) == 16 && sizeof (Devtable) == PAGE_SIZE (0) << ord_dev);

        /*
         * Command
         */
        class Cmd : public Queue::Entry
        {
            protected:
                enum class Type : uint8_t               // 4-bit Opcode
                {
                    COMPLETION_WAIT             = 0x1,  // 2.4.1
                    INVALIDATE_DEVTAB_ENTRY     = 0x2,  // 2.4.2
                    INVALIDATE_IOMMU_PAGES      = 0x3,  // 2.4.3
                    INVALIDATE_IOTLB_PAGES      = 0x4,  // 2.4.4
                    INVALIDATE_INTERRUPT_TABLE  = 0x5,  // 2.4.5
                    PREFETCH_IOMMU_PAGES        = 0x6,  // 2.4.6
                    COMPLETE_PPR_REQUEST        = 0x7,  // 2.4.7
                    INVALIDATE_IOMMU_ALL        = 0x8,  // 2.4.8
                    INSERT_GUEST_EVENT          = 0x9,  // 2.4.9
                    RESET_VMMIO                 = 0xa,  // 2.4.10
                };

                explicit constexpr Cmd (Type t, uint64_t l = 0, uint64_t h = 0) : Queue::Entry { uint64_t { std::to_underlying (t) } << 60 | (l & BITN_RANGE (59, 0)), h } {}

            public:
                auto type() const { return Type { static_cast<uint8_t>(lo >> 60 & BIT_RANGE (3, 0)) }; }
        };

        static_assert (__is_standard_layout (Cmd) && alignof (Cmd) == alignof (Queue::Entry) && sizeof (Cmd) == sizeof (Queue::Entry));

        struct Cmd_completion_wait : Cmd
        {
            explicit constexpr Cmd_completion_wait() : Cmd { Type::COMPLETION_WAIT } {}
            explicit constexpr Cmd_completion_wait (uint64_t volatile &var, uint64_t data) : Cmd { Type::COMPLETION_WAIT, Cpu::loc_to_phys (&var) | BIT (0), data } {}
        };

        struct Cmd_invalidate_devtab_entry : Cmd
        {
            explicit constexpr Cmd_invalidate_devtab_entry (uint16_t bdf) : Cmd { Type::INVALIDATE_DEVTAB_ENTRY, bdf } {}
        };

        struct Cmd_invalidate_iommu_pages : Cmd
        {
            explicit constexpr Cmd_invalidate_iommu_pages (uint16_t dom) : Cmd { Type::INVALIDATE_IOMMU_PAGES, uint64_t { dom } << 32, BITN_RANGE (63, 12) | BIT_RANGE (1, 0) } {}
        };

        struct Cmd_invalidate_interrupt_table : Cmd
        {
            explicit constexpr Cmd_invalidate_interrupt_table (uint16_t bdf) : Cmd { Type::INVALIDATE_INTERRUPT_TABLE, bdf } {}
        };

        struct Cmd_invalidate_iommu_all : Cmd
        {
            explicit constexpr Cmd_invalidate_iommu_all() : Cmd { Type::INVALIDATE_IOMMU_ALL } {}
        };

        /*
         * Event
         */
        class Evt final : public Queue::Entry
        {
            public:
                enum class Type : uint8_t               // 4-bit Event Code
                {
                    ILLEGAL_DEV_TABLE_ENTRY     = 0x1,  // 2.5.2
                    IO_PAGE_FAULT               = 0x2,  // 2.5.3
                    DEV_TAB_HARDWARE_ERROR      = 0x3,  // 2.5.4
                    PAGE_TAB_HARDWARE_ERROR     = 0x4,  // 2.5.5
                    ILLEGAL_COMMAND_ERROR       = 0x5,  // 2.5.6
                    COMMAND_HARDWARE_ERROR      = 0x6,  // 2.5.7
                    IOTLB_INV_TIMEOUT           = 0x7,  // 2.5.8
                    INVALID_DEVICE_REQUEST      = 0x8,  // 2.5.9
                    INVALID_PPR_REQUEST         = 0x9,  // 2.5.10
                    EVENT_COUNTER_ZERO          = 0xa,  // 2.5.11
                    GUEST_EVENT_FAULT           = 0xb,  // 2.5.12
                    VIOMMU_HARDWARE_ERROR       = 0xc,  // 2.5.13
                    RMP_PAGE_FAULT              = 0xd,  // 2.5.14
                    RMP_HARDWARE_ERROR          = 0xe,  // 2.5.15
                };

                auto type() const { return Type { static_cast<uint8_t>(lo >> 60 & BIT_RANGE (3, 0)) }; }
                auto eop1() const { return lo & BITN_RANGE (59, 0); }
                auto eop2() const { return hi; }
        };

        static_assert (__is_standard_layout (Evt) && alignof (Evt) == alignof (Queue::Entry) && sizeof (Evt) == sizeof (Queue::Entry));

        uint64_t const  efr1;       // IOMMU Features
        uint64_t const  efr2;       // IOMMU Features
        Devtable *const dtbl;       // DEV Table
        Queue           cmdq;       // CMD Queue
        Queue           evtq;       // EVT Queue

        // CPU-Local Completion Infrastructure
        static uint64_t sequence CPULOCAL;  // Sequence Number
        static uint64_t doorbell CPULOCAL;  // Doorbell

        bool feat_ia() const { return efr1 & BIT (6); }     // Supports INVALIDATE_IOMMU_ALL command
        bool feat_xt() const { return efr1 & BIT (2); }     // Supports x2APIC

        // Number of levels from HATS[1:0]
        unsigned lev() const { return 4 + min (static_cast<unsigned>(efr1 >> 10) & BIT_RANGE (1, 0), 2U); }

        [[nodiscard]] bool control (uint64_t c, uint64_t s) const
        {
            write (Reg64::CTL, c);

            // Wait for all relevant status bits to be set
            return Wait::until (timeout, [&] { return (read (Reg64::STS) & s) == s; });
        }

        void restart (uint64_t e) const
        {
            if (e) [[unlikely]] {

                auto const c { read (Reg64::CTL) };

                // Disable: 1->0 (Halt)
                write (Reg64::CTL, c ^ e);

                // Before restarting EVT_LOG (which clears EVT_OVR), wait for it to halt
                if (e & CTL::E_EVT_LOG) [[unlikely]]
                    Wait::until (timeout, [&] { return !(read (Reg64::STS) & STS::EVT_RUN); });

                // Enable: 0->1 (Restart)
                write (Reg64::CTL, c);
            }
        }

        [[nodiscard]] inline auto cmdq_submit (auto const &...c)
        {
            // Build a batch of cmd pointers from the fold expression
            Cmd const *cmd[] { &c... };

            // Submit the entire batch
            return cmdq_submit (cmd, sizeof...(c));
        }

        [[nodiscard]] bool cmdq_submit (Cmd const **c, size_t n)
        {
            auto const complete { ++sequence };

            {   Lock_guard <Spinlock> guard { cmdq.lock };

                // Wait until the queue has sufficient (n+1) capacity
                if (!Wait::until (timeout, [&] { return cmdq.num (cmdq_head(), true) > n; })) [[unlikely]]
                    return false;

                // Produce n commands
                while (n--)
                    cmdq.produce (**c++);

                // Append COMPLETION_WAIT
                cmdq.produce (Cmd_completion_wait { doorbell, complete });

                // Enforce ordering between queue writes (WB) and MMIO write (UC)
                Barrier::wmb();

                // Notify IOMMU
                write (Reg64::CMD_TAIL, cmdq.offs());
            }

            // Wait for completion
            return Wait::doorbell (timeout, doorbell, complete);
        }

        void handle_events()
        {
            Lock_guard <Spinlock> guard { evtq.lock };

            // Determine number of items to consume
            auto n { evtq.num (evtq_tail(), false) };

            // Prevent speculation on queue slots before tail has been determined
            Barrier::rmb();

            // Consume n events
            while (n--)
                event (static_cast<Evt const &>(evtq.consume()));

            // Notify IOMMU
            write (Reg64::EVT_HEAD, evtq.offs());
        }

        void event (Evt const &);

        [[nodiscard]] bool init() override final;

        void interrupt() override final;

        /*
         * DEV Invalidation
         */
        [[nodiscard]] bool invalidate_dev (uint16_t dom, uint16_t bdf) override final
        {
            return cmdq_submit (Cmd_invalidate_devtab_entry { bdf }, Cmd_invalidate_iommu_pages { dom });
        }

        /*
         * INT Invalidation
         */
        [[nodiscard]] bool invalidate_int (uint16_t bdf) override final
        {
            return cmdq_submit (Cmd_invalidate_interrupt_table { bdf });
        }

        /*
         * TLB Invalidation
         */
        [[nodiscard]] bool invalidate_tlb (uint16_t dom) override final
        {
            return cmdq_submit (Cmd_invalidate_iommu_pages { dom });
        }

        [[nodiscard]] bool invalidate_all()
        {
            // Fast path if INVALIDATE_IOMMU_ALL is supported
            if (feat_ia()) [[likely]]
                return cmdq_submit (Cmd_invalidate_iommu_all {});

            // Capacity of the command queue (leaving room for one completion)
            auto const cap { BIT (cmdq.ord) - 2 };

            // Execute INVALIDATE_DEVTAB_ENTRY for all valid BDFs
            for (size_t bdf { 0 }, num { dtbl->size() }; bdf < num; ) {

                // Populate cmdq with a batch of invalidations
                for (auto batch { min (num - bdf, cap) }; batch--; )
                    cmdq.produce (Cmd_invalidate_devtab_entry { static_cast<uint16_t>(bdf++) });

                // Submit the entire batch with a COMPLETION_WAIT command
                if (!cmdq_submit()) [[unlikely]]
                    return false;
            }

            // Execute INVALIDATE_IOMMU_PAGES for all valid DOMs
            for (size_t dom { 0 }, num { BIT (16) }; dom < num; ) {

                // Populate cmdq with a batch of invalidations
                for (auto batch { min (num - dom, cap) }; batch--; )
                    cmdq.produce (Cmd_invalidate_iommu_pages { static_cast<uint16_t>(dom++) });

                // Submit the entire batch with a COMPLETION_WAIT command
                if (!cmdq_submit()) [[unlikely]]
                    return false;
            }

            return true;
        }

        explicit Smmu_amd (uint64_t, pci_t, uint64_t, uint64_t, Devtable *);

        static Slab_cache cache;

        [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }

    public:
        [[nodiscard]] static Smmu_amd *create (uint64_t, pci_t, uint64_t, uint64_t);

        [[nodiscard]] Status assign_dev (Dc const *, Space_dma *, bool = true) override final;
};
