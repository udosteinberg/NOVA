/*
 * System Memory Management Unit (Arm SMMUv3)
 *
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

#include "slab.hpp"
#include "smmu.hpp"
#include "wait.hpp"

class Smmu_v3 final : private Smmu
{
    private:
        enum class Reg32 : unsigned
        {
            IDR0            = 0x00000,      // r-
            IDR1            = 0x00004,      // r-
            IDR2            = 0x00008,      // r-
            IDR3            = 0x0000c,      // r-
            IDR4            = 0x00010,      // r-
            IDR5            = 0x00014,      // r-
            IIDR            = 0x00018,      // r-
            AIDR            = 0x0001c,      // r-
            CR0             = 0x00020,      // rw
            CR0ACK          = 0x00024,      // r-
            CR1             = 0x00028,      // rw
            CR2             = 0x0002c,      // rw
            STATUSR         = 0x00040,      // r-
            GBPA            = 0x00044,      // rw
            AGBPA           = 0x00048,      // rw
            IRQ_CTRL        = 0x00050,      // rw
            IRQ_CTRLACK     = 0x00054,      // r-
            GERROR          = 0x00060,      // r-
            GERRORN         = 0x00064,      // rw
            GERROR_IRQ_CFG1 = 0x00070,      // rw
            GERROR_IRQ_CFG2 = 0x00074,      // rw
            STRTAB_BASE_CFG = 0x00088,      // rw
            CMDQ_PROD       = 0x00098,      // rw
            CMDQ_CONS       = 0x0009c,      // rw
            EVENTQ_PROD     = 0x100a8,      // rw
            EVENTQ_CONS     = 0x100ac,      // rw
            EVENTQ_IRQ_CFG1 = 0x000b8,      // rw
            EVENTQ_IRQ_CFG2 = 0x000bc,      // rw
            PRIQ_PROD       = 0x100c8,      // rw
            PRIQ_CONS       = 0x100cc,      // rw
            PRIQ_IRQ_CFG1   = 0x000d8,      // rw
            PRIQ_IRQ_CFG2   = 0x000dc,      // rw
            GATOS_CTRL      = 0x00100,      // rw
            MPAMIDR         = 0x00130,      // r-
            GMPAM           = 0x00138,      // rw
            GBPMPAM         = 0x0013c,      // rw
            VATOS_SEL       = 0x00180,      // rw
            IDR6            = 0x00190,      // r-
            DPT_BASE_CFG    = 0x00208,      // rw
        };

        enum class Reg64 : unsigned
        {
            S2PII           = 0x00030,      // rw
            GERROR_IRQ_CFG0 = 0x00068,      // rw
            STRTAB_BASE     = 0x00080,      // rw
            CMDQ_BASE       = 0x00090,      // rw
            EVENTQ_BASE     = 0x000a0,      // rw
            EVENTQ_IRQ_CFG0 = 0x000b0,      // rw
            PRIQ_BASE       = 0x000c0,      // rw
            PRIQ_IRQ_CFG0   = 0x000d0,      // rw
            GATOS_SID       = 0x00108,      // rw
            GATOS_ADDR      = 0x00110,      // rw
            GATOS_PAR       = 0x00118,      // r-
            DPT_BASE        = 0x00200,      // rw
            DPT_CFG_FAR     = 0x00210,      // rw
        };

        auto read  (Reg32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)); }
        auto read  (Reg64 r) const      { return *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r)); }
        void write (Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)) = v; }
        void write (Reg64 r, uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r)) = v; }

        [[nodiscard]] bool set_cr0 (uint32_t v) const
        {
            write (Reg32::CR0, v);

            // Wait until hardware acknowledges all bits
            return Wait::until (timeout, [&] { return read (Reg32::CR0ACK) == v; });
        }

        [[nodiscard]] bool set_gbpa (uint32_t v) const
        {
            write (Reg32::GBPA, BIT (31) | v);

            // Wait until hardware clears the update bit
            return Wait::until (timeout, [&] { return (read (Reg32::GBPA) & BIT (31)) == 0; });
        }

        [[nodiscard]] bool set_irq_ctrl (uint32_t v) const
        {
            write (Reg32::IRQ_CTRL, v);

            // Wait until hardware acknowledges all bits
            return Wait::until (timeout, [&] { return read (Reg32::IRQ_CTRLACK) == v; });
        }

        static void publish (void const *ptr, size_t size, bool coherent)
        {
            // A barrier is sufficient if SMMU is coherent
            if (coherent) [[likely]]
                return Barrier::wsb (Barrier::Domain::ISH);

            // Use 32 as conservative default because per-CPU line size is not yet available during init
            Cache::data_clean (ptr, size, 32);
        }

        // Command (4.1)
        class Cmd
        {
            private:
                uint64_t val[2] {};

            public:
                static constexpr unsigned ord { 0 };            // Page allocation order

                // Commands (4.1.1)
                enum class Type : uint8_t
                {
                    PREFETCH_CONFIG     = 0x01,
                    PREFETCH_ADDR       = 0x02,
                    CFGI_STE            = 0x03,
                    CFGI_STE_RANGE      = 0x04,
                    CFGI_CD             = 0x05,
                    CFGI_CD_ALL         = 0x06,
                    CFGI_VMS_PIDM       = 0x07,
                    TLBI_NH_ALL         = 0x10,                 // Stage-1 EL1  VMALLE1
                    TLBI_NH_ASID        = 0x11,                 // Stage-1 EL1  ASIDE1
                    TLBI_NH_VA          = 0x12,                 // Stage-1 EL1  VAE1
                    TLBI_NH_VAA         = 0x13,                 // Stage-1 EL1  VAAE1
                    TLBI_EL3_ALL        = 0x18,                 // CERROR_ILL   ALLE3
                    TLBI_EL3_VA         = 0x1a,                 // CERROR_ILL   VAE3
                    TLBI_EL2_ALL        = 0x20,                 // Stage-1 EL2  ALLE2
                    TLBI_EL2_ASID       = 0x21,                 // Stage-1 EL2
                    TLBI_EL2_VA         = 0x22,                 // Stage-1 EL2  VAE2
                    TLBI_EL2_VAA        = 0x23,                 // Stage-1 EL2
                    TLBI_S12_VMALL      = 0x28,                 // Stage-2      VMALLS12E1
                    TLBI_S2_IPA         = 0x2a,                 // Stage-2      IPAS2E1
                    TLBI_NSNH_ALL       = 0x30,                 // Common       ALLE1
                    ATC_INV             = 0x40,
                    PRI_RESP            = 0x41,
                    RESUME              = 0x44,
                    STALL_TERM          = 0x45,
                    SYNC                = 0x46,
                    TLBI_S_EL2_ALL      = 0x50,                 // CERROR_ILL
                    TLBI_S_EL2_ASID     = 0x51,                 // CERROR_ILL
                    TLBI_S_EL2_VA       = 0x52,                 // CERROR_ILL
                    TLBI_S_EL2_VAA      = 0x53,                 // CERROR_ILL
                    TLBI_S_S12_VMALL    = 0x58,                 // CERROR_ILL
                    TLBI_S_S2_IPA       = 0x5a,                 // CERROR_ILL
                    TLBI_SNH_ALL        = 0x60,                 // CERROR_ILL
                    DPTI_ALL            = 0x70,
                    DPTI_PA             = 0x73,
                };

                Cmd() = default;

                explicit constexpr Cmd (Type t, uint64_t l = 0, uint64_t h = 0) : val { l | std::to_underlying (t), h } {}
        };

        static_assert (__is_standard_layout (Cmd) && alignof (Cmd) == 8 && sizeof (Cmd) == 16);

        // 4.3.6
        struct Cmd_cfgi_all : Cmd
        {
            explicit constexpr Cmd_cfgi_all() : Cmd { Type::CFGI_STE_RANGE, 0, 31 } {}
        };

        static_assert (__is_standard_layout (Cmd_cfgi_all) && alignof (Cmd_cfgi_all) == alignof (Cmd) && sizeof (Cmd_cfgi_all) == sizeof (Cmd));

        // 4.4.2.7
        struct Cmd_tlbi_el2_all : Cmd
        {
            explicit constexpr Cmd_tlbi_el2_all() : Cmd { Type::TLBI_EL2_ALL } {}
        };

        static_assert (__is_standard_layout (Cmd_tlbi_el2_all) && alignof (Cmd_tlbi_el2_all) == alignof (Cmd) && sizeof (Cmd_tlbi_el2_all) == sizeof (Cmd));

        // 4.4.3.2
        struct Cmd_tlbi_s12_vmall : Cmd
        {
            explicit constexpr Cmd_tlbi_s12_vmall (uint16_t vmid) : Cmd { Type::TLBI_S12_VMALL, static_cast<uint64_t>(vmid) << 32 } {}
        };

        static_assert (__is_standard_layout (Cmd_tlbi_s12_vmall) && alignof (Cmd_tlbi_s12_vmall) == alignof (Cmd) && sizeof (Cmd_tlbi_s12_vmall) == sizeof (Cmd));

        // 4.4.4.1
        struct Cmd_tlbi_nsnh_all : Cmd
        {
            explicit constexpr Cmd_tlbi_nsnh_all() : Cmd { Type::TLBI_NSNH_ALL } {}
        };

        static_assert (__is_standard_layout (Cmd_tlbi_nsnh_all) && alignof (Cmd_tlbi_nsnh_all) == alignof (Cmd) && sizeof (Cmd_tlbi_nsnh_all) == sizeof (Cmd));

        // 4.7.3
        struct Cmd_sync : Cmd
        {
            explicit constexpr Cmd_sync() : Cmd { Type::SYNC } {}
        };

        static_assert (__is_standard_layout (Cmd_sync) && alignof (Cmd_sync) == alignof (Cmd) && sizeof (Cmd_sync) == sizeof (Cmd));

        // Event
        class Evt final
        {
            private:
                uint64_t val[4] {};

            public:
                static constexpr unsigned ord { 0 };            // Page allocation order

                // Events (7.3)
                enum class Type : uint8_t
                {
                    F_UUT               = 0x01,
                    C_BAD_STREAMID      = 0x02,
                    F_STE_FETCH         = 0x03,
                    C_BAD_STE           = 0x04,
                    F_BAD_ATS_TREQ      = 0x05,
                    F_STREAM_DISABLED   = 0x06,
                    F_TRANSL_FORBIDDEN  = 0x07,
                    C_BAD_SUBSTREAMID   = 0x08,
                    F_CD_FETCH          = 0x09,
                    C_BAD_CD            = 0x0a,
                    F_WALK_EABT         = 0x0b,
                    F_TRANSLATION       = 0x10,
                    F_ADDR_SIZE         = 0x11,
                    F_ACCESS            = 0x12,
                    F_PERMISSION        = 0x13,
                    F_TLB_CONFLICT      = 0x20,
                    F_CFG_CONFLICT      = 0x21,
                    E_PAGE_REQUEST      = 0x24,
                    F_VMS_FETCH         = 0x25,
                };
        };

        static_assert (__is_standard_layout (Evt) && alignof (Evt) == 8 && sizeof (Evt) == 32);

        // Typed FIFO
        template<typename T> struct Fifo
        {
            static constexpr unsigned qsz { PAGE_SIZE (0) << T::ord };
            static constexpr unsigned ord { bit_scan_msb (qsz / sizeof (T)) };

            T slot[BIT (ord)];

            [[nodiscard]] static void *operator new (size_t, bool c) noexcept
            {
                auto const ptr { Buddy::alloc (T::ord, Buddy::Fill::BITS0) };

                // Ensure writes are observable before the SMMU can see the table
                if (ptr) [[likely]]
                    publish (ptr, qsz, c);

                return ptr;
            }
        };

        static_assert (__is_standard_layout (Fifo<Cmd>) && alignof (Fifo<Cmd>) == alignof (Cmd) && sizeof (Fifo<Cmd>) == Fifo<Cmd>::qsz);
        static_assert (__is_standard_layout (Fifo<Evt>) && alignof (Fifo<Evt>) == alignof (Evt) && sizeof (Fifo<Evt>) == Fifo<Evt>::qsz);

        // Circular Queue (3.5.1)
        template<typename T, bool P> class Queue
        {
            private:
                using index_t = uint32_t;

                auto wrap (index_t i) const { return i &  BIT (ord); }              // Wrap Bit
                auto sidx (index_t i) const { return i & (BIT (ord + 0) - 1); }     // Slot Index
                auto qidx (index_t i) const { return i & (BIT (ord + 1) - 1); }     // Queue Index

            public:
                Fifo<T> * const ptr;        // FIFO Pointer
                unsigned  const ord;        // Order of Entries
                index_t         idx;        // Software Index

                auto get_base() const { return Kmem::ptr_to_phys (ptr); }
                auto get_qidx() const { return qidx (idx); }

                bool valid (index_t hwi) const
                {
                    bool const equal { wrap (idx) == wrap (hwi) };

                    // Partially full queue
                    if (sidx (idx) > sidx (hwi))
                        return equal == P;

                    // Partially full queue
                    if (sidx (idx) < sidx (hwi))
                        return equal != P;

                    // Empty or full queue
                    return true;
                }

                /*
                 * Produce a queue item
                 *
                 * @param t Item to put into queue
                 */
                void produce (T const &t, bool coherent)
                {
                    // Obtain slot pointer
                    auto const p { ptr->slot + sidx (idx++) };

                    // Put item into slot
                    *p = t;

                    // Ensure item observability
                    publish (p, sizeof (*p), coherent);
                }

                /*
                 * Consume a queue item
                 *
                 * @param t Item to get from queue
                 */
                void consume (T &t)
                {
                    // Obtain slot pointer
                    auto const p { ptr->slot + sidx (idx++) };

                    // Get item from slot
                    t = *p;
                }

                explicit constexpr Queue (Fifo<T> *p, unsigned o) : ptr { p }, ord { min (Fifo<T>::ord, o) }, idx { 0 } {}
        };

        // Stream Table Entry (5.2)
        class Ste final
        {
            private:
                uint64_t val[8] {};

            public:
                Ste() = default;

                explicit constexpr Ste (void *ptab, uint16_t vmid) : val {
                /* 063:000 */ BIT_RANGE (3, 2) | BIT (0),                                                       // Config=S2T+S1B, V=1
                /* 127:064 */ 0,
                /* 191:128 */ BIT64 (58) | BIT64 (51) | BIT64_RANGE (45, 44) | BIT64 (42) | BIT64 (40) | vmid,  // S2R=1, S2S=S2Hx=S2PTW=S2AFFD=S2ENDI=0, S2AA64=1, S2PS=FIXME, S2TG=4KB, S2SH0=IS, S2OR0=S2IR0=WB+RWA, S2SL0=FIXME, S2T0SZ=FIXME, VMID
                /* 255:192 */ Kmem::ptr_to_phys (ptab),                                                         // S2SKL=RES0, S2TTB, S2SL=S2NSA=RES0, S2NSW=RES0
                /* 319:256 */ 0,
                /* 383:320 */ 0,
                /* 447:384 */ 0,
                /* 511:448 */ 0 } {}

                // Allocate a table for 2^6 STEs
                [[nodiscard]] static void *operator new (size_t, bool c) noexcept
                {
                    auto const ptr { Buddy::alloc (0, Buddy::Fill::BITS0) };

                    // Ensure writes are observable before the SMMU can see the table
                    if (ptr) [[likely]]
                        publish (ptr, PAGE_SIZE (0), c);

                    return ptr;
                }
        };

        static_assert (__is_standard_layout (Ste) && alignof (Ste) == 8 && sizeof (Ste) == 64);

        // Stream Table Descriptor (5.1)
        class Std final
        {
            private:
                uint64_t val {};

            public:
                Std() = default;

                explicit constexpr Std (Ste *ptr) : val { Kmem::ptr_to_phys (ptr) | (ord_ste + 1) } {}

                // Return next-level stream table
                auto operator->() const { return static_cast<Ste *>(Kmem::phys_to_ptr (val & ~BIT_RANGE (5, 0))); }

                // Allocate a table for 2^o STDs
                [[nodiscard]] static void *operator new (size_t, bool c, unsigned o) noexcept
                {
                    auto const ord { static_cast<uint8_t>(max (o, ord_str) - ord_str) };
                    auto const ptr { Buddy::alloc (ord, Buddy::Fill::BITS0) };

                    // Ensure writes are observable before the SMMU can see the table
                    if (ptr) [[likely]]
                        publish (ptr, PAGE_SIZE (0) << ord, c);

                    return ptr;
                }
        };

        static_assert (__is_standard_layout (Std) && alignof (Std) == 8 && sizeof (Std) == 8);

        static constexpr unsigned ord_std { bit_scan_msb (PAGE_SIZE (0) / sizeof (Std)) };
        static constexpr unsigned ord_ste { bit_scan_msb (PAGE_SIZE (0) / sizeof (Ste)) };
        static constexpr unsigned ord_str { ord_std + ord_ste };
        static constexpr unsigned timeout { 10 };

        unsigned const      spi[4];
        uint32_t const      idr0;
        uint32_t const      idr1;
        uint32_t const      idr5;
        Queue<Cmd, true>    cmdq;                                       // Command queue
        Queue<Evt, false>   evtq;                                       // Event queue
        void *              ptr_str { nullptr };                        // Stream table

        bool sup_s2p() const { return BIT  (0) & idr0; }                // Supports stage-2 translation
        bool sup_s1p() const { return BIT  (1) & idr0; }                // Supports stage-1 translation
        bool sup_t32() const { return BIT  (2) & idr0; }                // Supports VMSAv8-32 (LPAE)
        bool sup_t64() const { return BIT  (3) & idr0; }                // Supports VMSAv8-64
        bool sup_coh() const { return BIT  (4) & idr0; }                // Supports coherent access to translations, structures and queues
        bool sup_btm() const { return BIT  (5) & idr0; }                // Supports broadcast TLB maintenance
        bool sup_hyp() const { return BIT  (9) & idr0; }                // Supports hypervisor stage-1 contexts
        bool sup_msi() const { return BIT (13) & idr0; }                // Supports message-signalled interrupts
        bool sup_pri() const { return BIT (16) & idr0; }                // Supports page request interface

        auto oas()     const { return BIT_RANGE (2, 0) & idr5; }
        bool sup_g04() const { return BIT (4) & idr5; }                 // Supports translation granule 4K
        bool sup_g16() const { return BIT (5) & idr5; }                 // Supports translation granule 16K
        bool sup_g64() const { return BIT (6) & idr5; }                 // Supports translation granule 64K

        auto str_lvl() const { return BIT_RANGE (1, 0) & idr0 >> 27; }  // [0, 1]

        auto hwo_cmd() const { return BIT_RANGE (4, 0) & idr1 >> 21; }  // Hardware Order: CMD Queue
        auto hwo_evt() const { return BIT_RANGE (4, 0) & idr1 >> 16; }  // Hardware Order: EVT Queue
        auto hwo_sid() const { return BIT_RANGE (5, 0) & idr1; }        // Hardware Order: Stream ID

        // Return chosen stream-table format (1 = 2-level, 0 = linear)
        auto str_fmt() const { return hwo_sid() > ord_ste ? 1 : 0; }

        explicit Smmu_v3 (uint64_t, unsigned, unsigned, unsigned, unsigned);

        uint8_t num_smg() const override final { return 0; }
        uint8_t num_ctx() const override final { return 0; }

        // FIXME: Locking
        bool command (Cmd const &cmd, bool sync)
        {
            auto const cons { read (Reg32::CMDQ_CONS) };

            // Command queue halted with error
            if (cons & BIT_RANGE (30, 24)) [[unlikely]]
                return false;

            // FIXME: Wait for queue space

            // Check queue validity
            assert (cmdq.valid (cons & BIT_RANGE (19, 0)));

            // Add command
            cmdq.produce (cmd, sup_coh());

            // Add sync command
            if (sync)
                cmdq.produce (Cmd_sync {}, sup_coh());

            // Check queue validity
            assert (cmdq.valid (cons & BIT_RANGE (19, 0)));

            // Notify SMMU about new commands
            write (Reg32::CMDQ_PROD, cmdq.get_qidx());

            // FIXME: Wait for completion

            return true;
        }

        bool cfg_invalidate_all()   { return               command (Cmd_cfgi_all      {}, true); }
        bool tlb_invalidate_alle2() { return !sup_hyp() || command (Cmd_tlbi_el2_all  {}, true); }
        bool tlb_invalidate_alle1() { return               command (Cmd_tlbi_nsnh_all {}, true); }

        bool init() override final;
        void fault() override final;
        void tlb_invalidate (Sdid) override final {}
        bool is_using_spi (unsigned s) const override final { return s == spi[0] || s == spi[1] || s == spi[2] || s == spi[3]; }

        Status assign_dev (Space_dma *, uintptr_t) override final { return Status::BAD_PAR; }

        static Slab_cache cache;

        [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }

    public:
        [[nodiscard]] static Smmu_v3 *setup (uint64_t, unsigned, unsigned, unsigned, unsigned);
};
