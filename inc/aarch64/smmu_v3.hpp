/*
 * System Memory Management Unit (Arm SMMUv3)
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

#include "coherence.hpp"
#include "endian.hpp"
#include "ptab_dpt.hpp"
#include "slab.hpp"
#include "smmu.hpp"

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

        auto read  (Reg32 r) const      { return *std::start_lifetime_as<uint32_t volatile> (mmio + std::to_underlying (r)); }
        auto read  (Reg64 r) const      { return *std::start_lifetime_as<uint64_t volatile> (mmio + std::to_underlying (r)); }

        void write (Reg32 r, uint32_t v) const { *std::start_lifetime_as<uint32_t volatile> (mmio + std::to_underlying (r)) = v; }
        void write (Reg64 r, uint64_t v) const { *std::start_lifetime_as<uint64_t volatile> (mmio + std::to_underlying (r)) = v; }

        [[nodiscard]] bool set_gbpa (uint32_t v) const
        {
            write (Reg32::GBPA, BIT (31) | v);

            // Wait until hardware clears the update bit
            return Wait::until (timeout, [&] { return (read (Reg32::GBPA) & BIT (31)) == 0; });
        }

        [[nodiscard]] bool set_cr0 (uint32_t v) const
        {
            write (Reg32::CR0, v);

            // Wait until hardware acknowledges all bits
            return Wait::until (timeout, [&] { return read (Reg32::CR0ACK) == v; });
        }

        [[nodiscard]] bool set_irq_ctrl (uint32_t v) const
        {
            write (Reg32::IRQ_CTRL, v);

            // Wait until hardware acknowledges all bits
            return Wait::until (timeout, [&] { return read (Reg32::IRQ_CTRLACK) == v; });
        }

        /*
         * Command
         */
        class Cmd
        {
            private:
                Aligned_le<uint64_t> w0, w1;        // Little-Endian

            public:
                enum class Type : uint8_t
                {
                    PREFETCH_CONFIG     = 0x01,     // 4.2.1
                    PREFETCH_ADDR       = 0x02,     // 4.2.2
                    CFGI_STE            = 0x03,     // 4.3.1
                    CFGI_STE_RANGE      = 0x04,     // 4.3.2
                    CFGI_CD             = 0x05,     // 4.3.3
                    CFGI_CD_ALL         = 0x06,     // 4.3.4
                    CFGI_VMS_PIDM       = 0x07,     // 4.3.5
                    TLBI_NH_ALL         = 0x10,     // 4.4.2.1      Stage-1 EL1  VMALLE1
                    TLBI_NH_ASID        = 0x11,     // 4.4.2.2      Stage-1 EL1  ASIDE1
                    TLBI_NH_VA          = 0x12,     // 4.4.2.4      Stage-1 EL1  VAE1
                    TLBI_NH_VAA         = 0x13,     // 4.4.2.3      Stage-1 EL1  VAAE1
                    TLBI_EL2_ALL        = 0x20,     // 4.4.2.7      Stage-1 EL2  ALLE2
                    TLBI_EL2_ASID       = 0x21,     // 4.4.2.10     Stage-1 EL2
                    TLBI_EL2_VA         = 0x22,     // 4.4.2.8      Stage-1 EL2  VAE2
                    TLBI_EL2_VAA        = 0x23,     // 4.4.2.9      Stage-1 EL2
                    TLBI_S12_VMALL      = 0x28,     // 4.4.3.2      Stage-2      VMALLS12E1
                    TLBI_S2_IPA         = 0x2a,     // 4.4.3.1      Stage-2      IPAS2E1
                    TLBI_NSNH_ALL       = 0x30,     // 4.4.4.1      Common       ALLE1
                    ATC_INV             = 0x40,     // 4.5.1
                    PRI_RESP            = 0x41,     // 4.5.2
                    RESUME              = 0x44,     // 4.7.1
                    STALL_TERM          = 0x45,     // 4.7.2
                    SYNC                = 0x46,     // 4.7.3
                    DPTI_ALL            = 0x70,     // 4.6.1
                    DPTI_PA             = 0x73,     // 4.6.2
                };

                auto type() const { return static_cast<uint8_t>(w0); }

                explicit constexpr Cmd() = default;

                explicit constexpr Cmd (Type t, uint64_t v0 = 0, uint64_t v1 = 0) : w0 { std::to_underlying (t) | v0 }, w1 { v1 } {}

                [[nodiscard]] static void *operator new[] (size_t s) noexcept { return Buddy::alloc (Buddy::size_to_ord (s)); }
        };

        static_assert (__is_standard_layout (Cmd) && alignof (Cmd) == 8 && sizeof (Cmd) == 16, "Invalid layout");

        struct Cmd_cfgi_ste : Cmd
        {
            explicit constexpr Cmd_cfgi_ste (uint32_t sid, bool leaf) : Cmd { Type::CFGI_STE, uint64_t { sid } << 32, leaf * BIT (0) } {}
        };

        struct Cmd_cfgi_all : Cmd
        {
            explicit constexpr Cmd_cfgi_all() : Cmd { Type::CFGI_STE_RANGE, 0, 31 } {}
        };

        struct Cmd_tlbi_el2_all : Cmd
        {
            explicit constexpr Cmd_tlbi_el2_all() : Cmd { Type::TLBI_EL2_ALL } {}
        };

        struct Cmd_tlbi_s12_vmall : Cmd
        {
            explicit constexpr Cmd_tlbi_s12_vmall (uint16_t vmid) : Cmd { Type::TLBI_S12_VMALL, uint64_t { vmid } << 32 } {}
        };

        struct Cmd_tlbi_nsnh_all : Cmd
        {
            explicit constexpr Cmd_tlbi_nsnh_all() : Cmd { Type::TLBI_NSNH_ALL } {}
        };

        struct Cmd_sync : Cmd
        {
            explicit constexpr Cmd_sync() : Cmd { Type::SYNC } {}
        };

        /*
         * Event
         */
        class Evt final
        {
            private:
                Aligned_le<uint64_t> w0, w1, w2, w3;    // Little-Endian

            public:
                enum class Type : uint8_t
                {
                    F_UUT               = 0x01,         // 7.3.2
                    C_BAD_STREAMID      = 0x02,         // 7.3.3
                    F_STE_FETCH         = 0x03,         // 7.3.4
                    C_BAD_STE           = 0x04,         // 7.3.5
                    F_BAD_ATS_TREQ      = 0x05,         // 7.3.6
                    F_STREAM_DISABLED   = 0x06,         // 7.3.7
                    F_TRANSL_FORBIDDEN  = 0x07,         // 7.3.8
                    C_BAD_SUBSTREAMID   = 0x08,         // 7.3.9
                    F_CD_FETCH          = 0x09,         // 7.3.10
                    C_BAD_CD            = 0x0a,         // 7.3.11
                    F_WALK_EABT         = 0x0b,         // 7.3.12
                    F_TRANSLATION       = 0x10,         // 7.3.13
                    F_ADDR_SIZE         = 0x11,         // 7.3.14
                    F_ACCESS            = 0x12,         // 7.3.15
                    F_PERMISSION        = 0x13,         // 7.3.16
                    F_TLB_CONFLICT      = 0x20,         // 7.3.17
                    F_CFG_CONFLICT      = 0x21,         // 7.3.18
                    E_PAGE_REQUEST      = 0x24,         // 7.3.19
                    F_VMS_FETCH         = 0x25,         // 7.3.20
                };

                auto iaddr() const { return static_cast<uint64_t>(w2); }

                auto sid() const { return static_cast<uint32_t>(w0 >> 32); }

                auto type() const { return Type { static_cast<uint8_t>(w0) }; }

                [[nodiscard]] static void *operator new[] (size_t s) noexcept { return Buddy::alloc (Buddy::size_to_ord (s)); }
        };

        static_assert (__is_standard_layout (Evt) && alignof (Evt) == 8 && sizeof (Evt) == 32, "Invalid layout");

        /*
         * Queue (3.5)
         */
        template<typename T> class Queue
        {
            private:
                using index_t = uint32_t;

                auto wbit (index_t i) const { return i &  BIT (ord); }          // Wrap Bit
                auto sidx (index_t i) const { return i & (BIT (ord) - 1); }     // Slot Idx

            public:
                unsigned  const ord;        // Order of Entries
                T *       const ptr;        // FIFO Pointer
                bool      const coh;        // Coherent Access
                index_t         swi;        // Software Index
                Spinlock        lock;       // Queue Lock

                auto get_idx() const { return swi & (BIT (ord + 1) - 1); }      // Full Idx
                auto get_ptr() const { return Kmem::ptr_to_phys (ptr); }        // Base Ptr

                bool is_empty (index_t hwi) const { return sidx (swi) == sidx (hwi) && wbit (swi) == wbit (hwi); }
                bool is_full  (index_t hwi) const { return sidx (swi) == sidx (hwi) && wbit (swi) != wbit (hwi); }

                void consume (T &t)
                {
                    // Obtain slot pointer and advance index
                    auto const p { ptr + sidx (swi++) };

                    Coherence::consumer (!coh, p, sizeof (*p));

                    // Get item from slot
                    t = *p;
                }

                void produce (T const &t)
                {
                    // Obtain slot pointer and advance index
                    auto const p { ptr + sidx (swi++) };

                    // Put item into slot
                    *p = t;

                    // Make item observable by SMMU
                    Coherence::producer (!coh, p, sizeof (*p));
                }

                void replace (index_t hwi, T &t) const
                {
                    // Get slot pointer for hardware index
                    auto const p { ptr + sidx (hwi) };

                    // Swap with item in slot
                    std::swap (*p, t);

                    // Make item observable by SMMU
                    Coherence::producer (!coh, p, sizeof (*p));
                }

                static constexpr unsigned swo { bit_scan_msb (PAGE_SIZE (0) / sizeof (T)) };

                // No need to make unused queue items observable by SMMU upfront
                explicit Queue (bool c, unsigned hwo) : ord { min (swo, hwo) }, ptr { new T[BIT (ord)] }, coh { c }, swi { 0 } {}
        };

        class Streamtable
        {
            public:
                /*
                 * L0 Stream Table Entry (5.2)
                 */
                class Ste final
                {
                    private:
                        /*
                         * See 3.21.3 for details on caching/invalidation and update procedures
                         */
                        mutable Atomic<uint64_t> val[8];

                        static constexpr uint64_t lock_bit { BIT (20) };

                    public:
                        void lock() const { while (val[1].test_and_set (lock_bit)) pause(); }

                        void unlock() const { val[1] &= ~lock_bit; }

                        bool valid() const { return val[0] & BIT (0); }

                        auto vmid() const { return static_cast<uint16_t>(val[2]); }

                        void detach (bool coherent, auto const &stei, auto const &tlbi)
                        {
                            assert (val[0] == (BIT_RANGE (3, 2) | BIT (0)) && val[1] == lock_bit && !val[4] && !val[5] && !val[6] && !val[7]);

                            // Valid -> Invalid
                            val[0] = 0;

                            // Make update observable by SMMU
                            Coherence::producer (!coherent, &val[0], sizeof (*val));
                            stei();     // Invalidate STE
                            tlbi();     // Invalidate TLB
                        }

                        void attach (bool coherent, auto const &stei, Atomic<Dpt::Entry> const *ptr, unsigned ptl, uint16_t vmid, unsigned pas, unsigned isz)
                        {
                            assert (!val[0] && val[1] == lock_bit && !val[4] && !val[5] && !val[6] && !val[7]);

                            //       S2R        | S2AA64     | S2PS                   | S2SH0                | S2OR0      | S2IR0      | S2SL0                      | S2T0SZ                      | S2VMID
                            val[2] = BIT64 (58) | BIT64 (51) | uint64_t { pas } << 48 | BIT64_RANGE (45, 44) | BIT64 (42) | BIT64 (40) | uint64_t { ptl - 2 } << 38 | uint64_t { 64 - isz } << 32 | vmid;

                            //       S2TTB
                            val[3] = Kmem::ptr_to_phys (ptr);

                            // Make update observable by SMMU
                            Coherence::producer (!coherent, &val[2], sizeof (*val) * 2);
                            stei();     // Invalidate STE

                            // Invalid -> Valid
                            val[0] = BIT_RANGE (3, 2) | BIT (0);

                            // Make update observable by SMMU
                            Coherence::producer (!coherent, &val[0], sizeof (*val));
                            stei();     // Invalidate STE
                        }

                        // Allocate storage for 2^6 STEs
                        [[nodiscard]] static void *operator new (size_t, bool c) noexcept
                        {
                            auto const ptr { Buddy::alloc (0, Buddy::Fill::BITS0) };

                            // Make invalid STEs observable by SMMU before table is installed
                            if (ptr) [[likely]]
                                Coherence::producer (!c, ptr, PAGE_SIZE (0));

                            return ptr;
                        }

                        static void operator delete (void *ptr) { Buddy::free (ptr); }
                };

                static_assert (__is_standard_layout (Ste) && alignof (Ste) == 8 && sizeof (Ste) == 64, "Invalid layout");

                /*
                 * L1 Stream Table Descriptor (5.1)
                 */
                class Std final
                {
                    private:
                        Atomic<uint64_t> val;

                    public:
                        [[nodiscard]] Ste *table (bool coherent, auto const &stdi)
                        {
                            for (uint64_t v { val };;) {

                                // Return existing stream table
                                if (v) [[likely]]
                                    return Kmem::phys_to_ptr<Ste> (v & BIT64_RANGE (55, 6));

                                // Allocate a new stream table
                                auto const ste { new (coherent) Ste };
                                if (!ste) [[unlikely]]
                                    return nullptr;

                                // Attempt to install the new stream table
                                if (!val.compare_exchange_n (v, Kmem::ptr_to_phys (ste) | (ord_ste + 1))) {
                                    delete ste;
                                    continue;
                                }

                                // Make STD update observable by SMMU
                                Coherence::producer (!coherent, this, sizeof (*this));

                                // STD invalidation (after update)
                                stdi();

                                // Return new stream table
                                return ste;
                            }
                        }

                        // Allocate storage for 2^o STDs
                        [[nodiscard]] static void *operator new (size_t, bool c, unsigned o) noexcept
                        {
                            auto const ord { static_cast<uint8_t>(max (o, ord_str) - ord_str) };
                            auto const ptr { Buddy::alloc (ord, Buddy::Fill::BITS0) };

                            // Make invalid STDs observable by SMMU before table is installed
                            if (ptr) [[likely]]
                                Coherence::producer (!c, ptr, PAGE_SIZE (0) << ord);

                            return ptr;
                        }
                };

                static_assert (__is_standard_layout (Std) && alignof (Std) == 8 && sizeof (Std) == 8, "Invalid layout");

                static constexpr unsigned ord_ste { bit_scan_msb (PAGE_SIZE (0) / sizeof (Ste)) };  //  6
                static constexpr unsigned ord_std { bit_scan_msb (PAGE_SIZE (0) / sizeof (Std)) };  //  9
                static constexpr unsigned ord_str { ord_std + ord_ste };                            // 15

                static_assert (ord_ste == 6 || ord_ste == 8 || ord_ste == 10, "Invalid split point");

            public:
                unsigned  const ord;        // Order of Entries
                unsigned  const lev;        // Max Level (1 = 2-level, 0 = linear)
                void *    const ptr;        // Table Pointer

                auto get_ptr() const { return Kmem::ptr_to_phys (ptr); }
                auto get_cfg() const { return lev << 16 | ord_ste << 6 | ord; }

                [[nodiscard]] Ste *entry (bool coherent, auto const &stdi, uint32_t sid)
                {
                    auto p { ptr };

                    switch (lev) {

                        default:
                            return nullptr;

                        case 1:
                            // Fail if L1 table does not exist
                            if (!p) [[unlikely]]
                                return nullptr;

                            // Get L0 table pointer
                            p = (static_cast<Std *>(p) + (sid >> ord_ste) % BIT (ord - ord_ste))->table (coherent, stdi);
                            [[fallthrough]];

                        case 0:
                            // Fail if L0 table does not exist
                            if (!p) [[unlikely]]
                                return nullptr;

                            // Get stream table entry
                            return static_cast<Ste *>(p) + sid % BIT (ord_ste);
                    }
                }

                static constexpr unsigned swo { 24 };   // FIXME: Depends on buddy max allocation size (2M)

                explicit Streamtable (bool c, unsigned hwo) : ord { min (swo, hwo) }, lev { ord > ord_ste }, ptr { lev ? static_cast<void *>(new (c, ord) Std) : static_cast<void *>(new (c) Ste) } {}
        };

        uint32_t const      intid[4];   // EVT, PRI, GLB, CMD
        uint32_t const      idr0;
        uint32_t const      idr1;
        uint32_t const      idr5;
        Queue<Cmd>          cmdq;       // Command queue
        Queue<Evt>          evtq;       // Event queue
        Streamtable         strt;       // Stream table

        [[nodiscard]] constexpr bool feat_s2p()    const { return idr0 & BIT  (0); }    // Supports stage-2 translation
        [[nodiscard]] constexpr bool feat_s1p()    const { return idr0 & BIT  (1); }    // Supports stage-1 translation
        [[nodiscard]] constexpr bool feat_t64()    const { return idr0 & BIT  (3); }    // Supports VMSAv8-64
        [[nodiscard]] constexpr bool coherent()    const { return idr0 & BIT  (4); }    // Supports coherent access to translations, structures and queues
        [[nodiscard]] constexpr bool feat_btm()    const { return idr0 & BIT  (5); }    // Supports broadcast TLB maintenance
        [[nodiscard]] constexpr bool feat_hyp()    const { return idr0 & BIT  (9); }    // Supports hypervisor stage-1 contexts
        [[nodiscard]] constexpr bool feat_msi()    const { return idr0 & BIT (13); }    // Supports message-signalled interrupts
        [[nodiscard]] constexpr bool feat_pri()    const { return idr0 & BIT (16); }    // Supports page request interface
        [[nodiscard]] constexpr bool feat_vmid16() const { return idr0 & BIT (18); }    // Supports 16-bit VMIDs

        [[nodiscard]] constexpr auto oas()      const { return idr5 & BIT_RANGE (2, 0); }       // Output address size
        [[nodiscard]] constexpr bool feat_g04() const { return idr5 & BIT (4); }                // Supports translation granule 4K

        [[nodiscard]] constexpr auto hwo_cmd() const { return BIT_RANGE (4, 0) & idr1 >> 21; }  // Hardware Order: CMD Queue
        [[nodiscard]] constexpr auto hwo_evt() const { return BIT_RANGE (4, 0) & idr1 >> 16; }  // Hardware Order: EVT Queue
        [[nodiscard]] constexpr auto hwo_sid() const { return BIT_RANGE (5, 0) & idr1; }        // Hardware Order: Stream ID

        uint8_t num_smg() const override final { return 0; }
        uint8_t num_ctx() const override final { return 0; }

        [[nodiscard]] bool command (Cmd const &cmd, bool sync)
        {
            Lock_guard <Spinlock> guard { cmdq.lock };

            // Command queue should be empty
            assert (cmdq.is_empty (read (Reg32::CMDQ_CONS) & BIT_RANGE (19, 0)));

            // Add command
            cmdq.produce (cmd);

            // Add sync command
            if (sync) [[likely]]
                cmdq.produce (Cmd_sync {});

            // Producer notification to SMMU
            write (Reg32::CMDQ_PROD, cmdq.get_idx());

            // Wait for SMMU completion
            return Wait::until (timeout, [&] { return cmdq.is_empty (read (Reg32::CMDQ_CONS) & BIT_RANGE (19, 0)); });
        }

        [[nodiscard]] bool cfgi_ste (uint32_t sid, bool leaf)
        {
            return command (Cmd_cfgi_ste { sid, leaf }, true);
        }

        [[nodiscard]] bool cfgi_all()
        {
            return command (Cmd_cfgi_all {}, true);
        }

        [[nodiscard]] bool tlbi_alle2()
        {
            // Valid only if hypervisor translation is supported
            if (!feat_hyp()) [[unlikely]]
                return true;

            return command (Cmd_tlbi_el2_all {}, true);
        }

        [[nodiscard]] bool tlbi_alle1()
        {
            // Valid only if stage-1 or stage-2 translation is supported
            if (!feat_s1p() && !feat_s2p()) [[unlikely]]
                return true;

            return command (Cmd_tlbi_nsnh_all {}, true);
        }

        [[nodiscard]] bool tlbi_vmalls12e1 (uint16_t vmid)
        {
            // Valid only if stage-2 translation is supported
            if (!feat_s2p()) [[unlikely]]
                return true;

            return command (Cmd_tlbi_s12_vmall { vmid }, true);
        }

        bool invalidate_tlb (uint16_t vmid) override final
        {
            return tlbi_vmalls12e1 (vmid);
        }

        /*
         * Event queue notification
         *
         * Only invoked by the one CPU to which the interrupt is routed
         */
        void notify_evt()
        {
            auto const prod { read (Reg32::EVENTQ_PROD) };

            // Handle events
            handle_evt (prod & BIT_RANGE (19, 0));

            // Notification to SMMU and acknowledge overflow
            write (Reg32::EVENTQ_CONS, (prod & BIT (31)) | evtq.get_idx());
        }

        /*
         * Global error notification
         *
         * Only invoked by the one CPU to which the interrupt is routed
         */
        void notify_glb()
        {
            auto const err { read (Reg32::GERROR) };
            auto const ack { read (Reg32::GERRORN) };

            // Handle errors
            handle_glb (err ^ ack);

            // Acknowledge errors
            write (Reg32::GERRORN, err);
        }

        void fault (uint32_t iid) override final
        {
            if (intid[0] == iid)
                notify_evt();

            if (intid[2] == iid)
                notify_glb();
        }

        bool is_using_iid (uint32_t iid) const override final
        {
            return intid[0] == iid || intid[1] == iid || intid[2] == iid || intid[3] == iid;
        }

        void handle_evt (uint32_t);
        void handle_glb (uint32_t);

        [[nodiscard]] bool init() override final;

        [[nodiscard]] Status assign_dev (Dc_state const *, Space_dma *, Space_dma *, uintptr_t &) override final;

        [[nodiscard]] explicit Smmu_v3 (uint64_t, uint32_t const (&)[4]);

        [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }

        static Slab_cache cache;

    public:
        [[nodiscard]] static Smmu_v3 *create (uint64_t, uint32_t const (&)[4]);
};
