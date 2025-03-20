/*
 * GIC Interrupt Translation Service (GITS)
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

#include "coherency.hpp"
#include "coresight.hpp"
#include "endian.hpp"
#include "intid.hpp"
#include "list.hpp"
#include "lock_guard.hpp"
#include "mmio.hpp"
#include "slab.hpp"
#include "status.hpp"
#include "wait.hpp"

class Gits final : public List<Gits>, private Coresight, private Intid, private Mmio
{
    friend class Acpi_table_madt;

    private:
        enum class Reg32 : unsigned
        {
            CTLR            = 0x00000,  // -- v3 rw Control Register
            IIDR            = 0x00004,  // -- v3 r- Identification Register
            MPAMIDR         = 0x00010,  // -- v3 r- Supported MPAM Size
            PARTIDR         = 0x00014,  // -- v3 rw PARTID Register
            MPIDR           = 0x00018,  // -- v3 r- Affinity
            STATUSR         = 0x00040,  // -- v3 rw Error Reporting Status Register
            TRANSLATER      = 0x10040,  // -- v3 -w ITS Translation Register
        };

        enum class Reg64 : unsigned
        {
            TYPER           = 0x00008,  // -- v3 r- Type Register
            UMSIR           = 0x00048,  // -- v3 r- Unmapped MSI Register
            CBASER          = 0x00080,  // -- v3 rw Command Queue Descriptor
            CWRITER         = 0x00088,  // -- v3 rw Write Register
            CREADR          = 0x00090,  // -- v3 r- Read Register
            SGIR            = 0x20020,  // -- v3 -w SGI Register
        };

        enum class Arr64 : unsigned
        {
            BASER           = 0x00100,  // -- v3 rw Translation Table Descriptors
        };

        auto read  (Reg32 r)             const      { return *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)); }
        auto read  (Reg64 r)             const      { return *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r)); }
        auto read  (Arr64 r, unsigned n) const      { return *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r) + n * sizeof (uint64_t)); }

        void write (Reg32 r,             uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)) = v; }
        void write (Reg64 r,             uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r)) = v; }
        void write (Arr64 r, unsigned n, uint64_t v) const { *reinterpret_cast<uint64_t volatile *>(mmio + std::to_underlying (r) + n * sizeof (uint64_t)) = v; }

        // Timeout for GITS hardware operations
        static constexpr unsigned timeout { 1 };

        [[nodiscard]] bool set_ctlr (bool enable) const
        {
            write (Reg32::CTLR, enable * BIT (0));

            // Ensure ITS is quiescent after disabling
            return enable || Wait::until (timeout, [&] { return read (Reg32::CTLR) & BIT (31); });
        }

        /*
         * ITS Command
         */
        struct Cmd
        {
            Aligned_le<uint64_t> w0, w1, w2, w3;    // Little-Endian

            enum class Type : uint8_t
            {
                MOVI                = 0x01,         // 5.3.14
                INT                 = 0x03,         // 5.3.5
                CLEAR               = 0x04,         // 5.3.3
                SYNC                = 0x05,         // 5.3.15
                MAPD                = 0x08,         // 5.3.10
                MAPC                = 0x09,         // 5.3.9
                MAPTI               = 0x0a,         // 5.3.12
                MAPI                = 0x0b,         // 5.3.11
                INV                 = 0x0c,         // 5.3.6
                INVALL              = 0x0d,         // 5.3.7
                MOVALL              = 0x0e,         // 5.3.13
                DISCARD             = 0x0f,         // 5.3.4
                VMOVI               = 0x21,         // 5.3.21
                VMOVP               = 0x22,         // 5.3.22 / 5.3.23
                VSGI                = 0x23,         // 5.3.24
                VSYNC               = 0x25,         // 5.3.25
                VMAPP               = 0x29,         // 5.3.18 / 5.3.19
                VMAPTI              = 0x2a,         // 5.3.20
                VMAPI               = 0x2b,         // 5.3.17
                VINVALL             = 0x2d,         // 5.3.16
                INVDB               = 0x2e,         // 5.3.8
            };

            explicit constexpr Cmd (Type t, uint64_t v0 = 0, uint64_t v1 = 0, uint64_t v2 = 0, uint64_t v3 = 0) : w0 { std::to_underlying (t) | v0 }, w1 { v1 }, w2 { v2 }, w3 { v3 } {}
        };

        static_assert (__is_standard_layout (Cmd) && alignof (Cmd) == 8 && sizeof (Cmd) == 32, "Invalid layout");

        struct Cmd_discard : Cmd
        {
            explicit constexpr Cmd_discard (arm_devid_t did, arm_evtid_t eid) : Cmd { Type::DISCARD, uint64_t { did } << 32, eid } {}
        };

        struct Cmd_invall : Cmd
        {
            explicit constexpr Cmd_invall (arm_colid_t cid) : Cmd { Type::INVALL, 0, 0, cid } {}
        };

        struct Cmd_mapc : Cmd
        {
            explicit constexpr Cmd_mapc (arm_colid_t cid, uint64_t rta) : Cmd { Type::MAPC, 0, 0, BIT64 (63) | rta | cid } {}
        };

        struct Cmd_mapd : Cmd
        {
            // V = 0: Remove device table mapping for did
            explicit constexpr Cmd_mapd (arm_devid_t did) : Cmd { Type::MAPD, uint64_t { did } << 32 } {}

            // V = 1: Insert device table mapping for did with itt:ord
            explicit constexpr Cmd_mapd (arm_devid_t did, uint64_t itt, unsigned ord) : Cmd { Type::MAPD, uint64_t { did } << 32, ord - 1, BIT64 (63) | itt } {}
        };

        struct Cmd_mapti : Cmd
        {
            explicit constexpr Cmd_mapti (arm_devid_t did, arm_evtid_t eid, arm_intid_t iid, arm_colid_t cid) : Cmd { Type::MAPTI, uint64_t { did } << 32, uint64_t { iid } << 32 | eid, cid } {}
        };

        struct Cmd_sync : Cmd
        {
            explicit constexpr Cmd_sync (uint64_t rta) : Cmd { Type::SYNC, 0, 0, rta } {}
        };

        /*
         * ITS Command Queue (5.2.8)
         */
        class Queue
        {
            private:
                using index_t = uint32_t;

                auto sidx (index_t i) const { return i & (BIT (ord) - 1); }     // Slot Idx

            public:
                Cmd * const     ptr;        // FIFO Pointer
                unsigned const  ord;        // Order of Entries
                index_t         swi;        // Software Index
                Spinlock        lock;       // Queue Lock

                auto get_idx() const { return sidx (swi); }
                auto get_ptr() const { return Kmem::ptr_to_phys (ptr); }

                bool is_empty (index_t hwi) const { return sidx (swi) == hwi; }

                void produce (Cmd const &cmd, bool coh)
                {
                    // Obtain slot pointer and advance index
                    auto const p { ptr + sidx (swi++) };

                    // Put command into slot
                    *p = cmd;

                    // Make command observable
                    Coherency::observe (p, sizeof (*p), coh);
                }

                void replace (Cmd &cmd, bool coh, index_t hwi) const
                {
                    // Get slot pointer for hardware index
                    auto const p { ptr + hwi };

                    // Swap with command in slot
                    std::swap (*p, cmd);

                    // Make command observable
                    Coherency::observe (p, sizeof (*p), coh);
                }

                // Page allocation order (64K)
                static constexpr unsigned pao { 4 };

                // No need to make producer-owned queue items observable upfront
                explicit Queue() : ptr { static_cast<Cmd *>(Buddy::alloc (pao, Buddy::Fill::BITS0)) }, ord { bit_scan_msb ((PAGE_SIZE (0) << pao) / sizeof (Cmd)) } {}
        };

        /*
         * ITS Table (5.2.1)
         */
        struct Table
        {
            private:
                Atomic<uint64_t> val;

            public:
                [[nodiscard]] bool allocate (unsigned o, bool c)
                {
                    // Valid leaf table orders are 0 (4K), 2 (16K), 4 (64K)
                    assert (o == 0 || o == 2 || o == 4);

                    uint64_t v { val };

                    // Not yet allocated
                    if (!v) [[unlikely]] {

                        // Allocate leaf table
                        auto const ptr { new (o, c) Table };

                        // Allocation failure
                        if (!ptr) [[unlikely]]
                            return false;

                        // Attempt to install the new leaf table
                        if (val.compare_exchange_n (v, BIT64 (63) | Kmem::ptr_to_phys (ptr))) [[likely]]
                            Coherency::observe (this, sizeof (*this), c);
                        else
                            delete ptr;
                    }

                    return true;
                }

                [[nodiscard]] static void *operator new (size_t, unsigned o, bool c) noexcept
                {
                    // Valid (concatenated) root table orders are 0 (4K) ... 12 (16M = 256 * 64K)
                    assert (o <= 12);

                    // Allocate table storage with all bytes zero
                    auto const ptr { Buddy::alloc (static_cast<Buddy::order_t>(o), Buddy::Fill::BITS0) };

                    // Make table observable
                    if (ptr) [[likely]]
                        Coherency::observe (ptr, PAGE_SIZE (0) << o, c);

                    return ptr;
                }

                static void operator delete (void *ptr) { Buddy::free (ptr); }
        };

        uint32_t const  id;
        uint64_t const  typer;
        uint64_t        baser_dev {};
        uint64_t        baser_col {};
        Queue           cmdq;

        static inline constinit Gits *list { nullptr };
        static inline constinit bool coherent { true };
        static Slab_cache cache;

        static constexpr auto impl_bits { VAL64_SHIFT (3, 10) | VAL64_SHIFT (7, 53) };  // Implementation-Defined Fixed-Value Bits
        static constexpr auto attr_isic { VAL64_SHIFT (1, 10) | VAL64_SHIFT (7, 59) };  // Inner Shareable + Inner Cacheable (RA/WA/WB)
        static constexpr auto attr_nsnc { VAL64_SHIFT (0, 10) | VAL64_SHIFT (1, 59) };  // Non-Shareable + Non-Cacheable

        bool feat_pta()  const { return BIT (19) & typer; }                             // Physical Target Address
        bool feat_vlpi() const { return BIT  (1) & typer; }                             // Supports Virt LPIs
        bool feat_plpi() const { return BIT  (0) & typer; }                             // Supports Phys LPIs

        auto num_hcc() const { return static_cast<unsigned>((BIT_RANGE (7, 0) & typer >> 24)); }        // Hardware Collection Count
        auto ord_dev() const { return static_cast<unsigned>((BIT_RANGE (4, 0) & typer >> 13) + 1); }    // Order: DevID bits
        auto ord_evt() const { return static_cast<unsigned>((BIT_RANGE (4, 0) & typer >>  8) + 1); }    // Order: EvtID bits
        auto esz_itt() const { return static_cast<unsigned>((BIT_RANGE (3, 0) & typer >>  4) + 1); }    // Entry Size: ITT

        // ITT order is constrained by supported EvtID bits and number of entries that fit into the allocated page
        auto ord_itt() const { return min (ord_evt(), static_cast<unsigned>(bit_scan_msb (PAGE_SIZE (0) / esz_itt()))); }

        // Redistributor Target Address is either GICR's 64K-aligned physical address or GICR's PE number shifted into position
        uint64_t rta (cpu_t cpu) const { assert (cpu < Cpu::count); return feat_pta() ? *Kmem::loc_to_glb (cpu, &Cpu::gicr) & BIT64_RANGE (51, 16) : *Kmem::loc_to_glb (cpu, &Cpu::gicr_pe) << 16; }

        explicit Gits (uint64_t, uint32_t);

        [[nodiscard]] bool command (Cmd const &);

        [[nodiscard]] bool init_baser (unsigned);

        [[nodiscard]] bool init_table (uint64_t, unsigned);

        [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }

    public:
        [[nodiscard]] bool init();

        [[nodiscard]] Status conf_lpi (arm_intid_t, arm_colid_t, arm_devid_t, arm_evtid_t, uintptr_t &, uintptr_t &);

        [[nodiscard]] void *itt_alloc (arm_devid_t did)
        {
            // Ensure device table entry for DID exists
            if (init_table (baser_dev, did)) [[likely]] {

                // Allocate ITT
                auto const itt { Buddy::alloc (0, Buddy::Fill::BITS0) };

                if (itt) [[likely]] {

                    trace (TRACE_INTR, "GITS: %#010lx MAPD DID:%#x => ITT:%#lx ORD:%u", phys, did, Kmem::ptr_to_phys (itt), ord_itt());

                    // Map DID => ITT:ORD
                    if (command (Cmd_mapd { did, Kmem::ptr_to_phys (itt), ord_itt() })) [[likely]]
                        return itt;

                    Buddy::free (itt);
                }
            }

            return nullptr;
        }

        void itt_free (arm_devid_t did, void *itt)
        {
            // Unmap and deallocate ITT
            if (itt && command (Cmd_mapd { did })) [[likely]]
                Buddy::free (itt);
        }

        [[nodiscard]] static auto setup (uint64_t p, uint32_t i) { return new Gits { p, i }; }

        [[nodiscard]] static bool initialize()
        {
            assert (Cpu::bsp);

            for (auto gits { list }; gits; gits = gits->next)
                if (!gits->init()) [[unlikely]]
                    return false;

            return true;
        }

        // Lookup GITS based on HPA
        [[nodiscard]] static Gits *lookup (uint64_t hpa)
        {
            for (auto gits { list }; gits; gits = gits->next)
                if (gits->phys == hpa)
                    return gits;

            return nullptr;
        }
};
