/*
 * Generic Interrupt Controller: Interrupt Translation Service (GITS)
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "intid.hpp"
#include "list.hpp"
#include "lock_guard.hpp"
#include "mmio.hpp"
#include "slab.hpp"
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
         * Command
         */
        class Cmd
        {
            private:
                uint64_t val[4];

            public:
                enum class Type : uint8_t
                {
                    MOVI                = 0x01,     // 5.3.14
                    INT                 = 0x03,     // 5.3.5
                    CLEAR               = 0x04,     // 5.3.3
                    SYNC                = 0x05,     // 5.3.15
                    MAPD                = 0x08,     // 5.3.10
                    MAPC                = 0x09,     // 5.3.9
                    MAPTI               = 0x0a,     // 5.3.12
                    MAPI                = 0x0b,     // 5.3.11
                    INV                 = 0x0c,     // 5.3.6
                    INVALL              = 0x0d,     // 5.3.7
                    MOVALL              = 0x0e,     // 5.3.13
                    DISCARD             = 0x0f,     // 5.3.4
                    VMOVI               = 0x21,     // 5.3.21
                    VMOVP               = 0x22,     // 5.3.22 / 5.3.23
                    VSGI                = 0x23,     // 5.3.24
                    VSYNC               = 0x25,     // 5.3.25
                    VMAPP               = 0x29,     // 5.3.18 / 5.3.19
                    VMAPTI              = 0x2a,     // 5.3.20
                    VMAPI               = 0x2b,     // 5.3.17
                    VINVALL             = 0x2d,     // 5.3.16
                    INVDB               = 0x2e,     // 5.3.8
                };

                explicit constexpr Cmd (Type t, uint64_t v0 = 0, uint64_t v1 = 0, uint64_t v2 = 0, uint64_t v3 = 0) : val { std::to_underlying (t) | v0, v1, v2, v3 } {}
        };

        static_assert (__is_standard_layout (Cmd) && alignof (Cmd) == 8 && sizeof (Cmd) == 32, "Invalid layout");

        /*
         * Map device table entry dev to associated ITT, defined by itt and ord
         */
        struct Cmd_mapd : Cmd
        {
            explicit constexpr Cmd_mapd (uint32_t dev, unsigned ord, uint64_t itt) : Cmd { Type::MAPD, uint64_t { dev } << 32, ord - 1, BIT64 (63) | itt } {}
        };

        /*
         * Map collection table entry col to target redistributor, defined by red
         */
        struct Cmd_mapc : Cmd
        {
            explicit constexpr Cmd_mapc (uint64_t red, uint16_t col) : Cmd { Type::MAPC, 0, 0, BIT64 (63) | (red & BIT64_RANGE (50, 16)) | col } {}
        };

        /*
         * Map event evt and device dev to associated ITT entry with collection col and INTID == evt
         */
        struct Cmd_mapi : Cmd
        {
            explicit constexpr Cmd_mapi (uint32_t dev, uint32_t evt, uint16_t col) : Cmd { Type::MAPI, uint64_t { dev } << 32, evt, col } {}
        };

        /*
         * Command Queue (5.2.8)
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

                void produce (Cmd const &c, bool coherent)
                {
                    // Obtain slot pointer and advance index
                    auto const p { ptr + sidx (swi++) };

                    // Put command into slot
                    *p = c;

                    // Make command observable by GITS
                    Coherency::observe (p, sizeof (*p), coherent);
                }

                // Page allocation order (64K)
                static constexpr unsigned pao { 4 };

                // No need to make unused queue items observable by GITS upfront
                explicit Queue() : ptr { static_cast<Cmd *>(Buddy::alloc (pao, Buddy::Fill::BITS0)) }, ord { bit_scan_msb ((PAGE_SIZE (0) << pao) / sizeof (Cmd)) } {}
        };

        uint64_t const  typer;
        uint64_t        baser[8];
        Queue           cmdq;

        static inline constinit Gits *list { nullptr };
        static inline constinit bool coherent { true };

        static constexpr auto impl_bits { VAL64_SHIFT (3, 10) | VAL64_SHIFT (7, 53) };  // Implementation-Defined Fixed-Value Bits
        static constexpr auto attr_isic { VAL64_SHIFT (1, 10) | VAL64_SHIFT (7, 59) };  // Inner Shareable + Inner Cacheable (RA/WA/WB)
        static constexpr auto attr_nsnc { VAL64_SHIFT (0, 10) | VAL64_SHIFT (1, 59) };  // Non-Shareable + Non-Cacheable

        bool feat_pta()     const { return BIT (19) & typer; }                          // Physical Target Address
        bool feat_vlpi()    const { return BIT  (1) & typer; }                          // Supports Virt LPIs
        bool feat_plpi()    const { return BIT  (0) & typer; }                          // Supports Phys LPIs

        auto hwo_dev() const { return static_cast<unsigned>(1 + (BIT_RANGE (4, 0) & typer >> 13)); }    // Hardware Order: DevID bits
        auto hwo_evt() const { return static_cast<unsigned>(1 + (BIT_RANGE (4, 0) & typer >>  8)); }    // Hardware Order: EvtID bits
        auto itt_ent() const { return static_cast<unsigned>(1 + (BIT_RANGE (3, 0) & typer >>  4)); }    // ITT Entry Size

        [[nodiscard]] bool command (Cmd const &cmd)
        {
            Lock_guard <Spinlock> guard { cmdq.lock };

            // Command queue should be empty
            assert (cmdq.is_empty (read (Reg64::CREADR) >> 5 & BIT_RANGE (14, 0)));

            // Add command
            cmdq.produce (cmd, coherent);

#if 0
            // Add sync command
            if (sync) [[likely]]
                cmdq.produce (Cmd_sync {});
#endif

            // Producer notification to GITS
            write (Reg64::CWRITER, cmdq.get_idx() << 5);

            // Wait for GITS completion
            return Wait::until (timeout, [&] { return cmdq.is_empty (read (Reg64::CREADR) >> 5 & BIT_RANGE (14, 0)); });
        }

        explicit Gits (uint64_t);

        static Slab_cache cache;

        [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }

    public:
        [[nodiscard]] bool init();

        [[nodiscard]] static auto setup (uint64_t base) { return new Gits { base }; }

        [[nodiscard]] static bool initialize()
        {
            if (Cpu::bsp)
                for (auto gits { list }; gits; gits = gits->next)
                    if (!gits->init()) [[unlikely]]
                        return false;

            return true;
        }
};
