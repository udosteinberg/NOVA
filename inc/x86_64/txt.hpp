/*
 * Trusted Execution Technology (TXT)
 *
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

#include "memory.hpp"
#include "std.hpp"
#include "uuid.hpp"

class Txt final
{
    private:
        static constexpr auto version_os_sinit { 7 };

        /*
         * 0xfed20000-0xfed2ffff:  64K TXT private
         * 0xfed30000-0xfed3ffff:  64K TXT public
         * 0xfed40000-0xfed44fff:  20K TPM localities
         * 0xfed45000-0xfed7ffff: 236K TXT reserved
         */
        static constexpr auto txt_base { 0xfed20000 };
        static constexpr auto txt_size { 0x60000 };

        struct Heap_alloc
        {
            [[nodiscard]] static void *operator new (size_t, uintptr_t p) noexcept { return reinterpret_cast<void *>(p); }
        };

        // Section C.1: Extended Heap Element
        struct Element
        {
            // Section C.5.5: Registry of Extended Heap Elements
            enum class Type : uint32_t
            {
                END         = 0,    // Terminator
                VER         = 1,    // TXT BIOS Specification Version
                ACM         = 2,    // ACM Information
                STM         = 3,    // STM Information
                CUSTOM      = 4,    // Customizable
                LOG12       = 5,    // TPM 1.2 Event Log
                MADT        = 6,    // Validated ACPI MADT
                LOG20_TXT   = 7,    // TPM 2.0 Event Log (TXT)
                LOG20_TCG   = 8,    // TPM 2.0 Event Log (TCG)
                MCFG        = 9,    // Validated ACPI MCFG
                TPR         = 13,   // TPR Request
                DPTR        = 14,   // Validated ACPI DPTR
                CEDT        = 15,   // Validated ACPI CEDT
            };

            Type     const  type;
            uint32_t const  size;

            auto data() const { return reinterpret_cast<uintptr_t>(this + 1); }
            auto next() const { return reinterpret_cast<Element const *>(reinterpret_cast<uintptr_t>(this) + size); }
        };

        static_assert (__is_standard_layout (Element) && sizeof (Element) == 8);

#pragma pack(1)

        // Extended Heap Element: Type 0
        struct Element_end : Heap_alloc
        {
            Element  const  elem        { Element::Type::END, sizeof (Element_end) };
        };

        static_assert (__is_standard_layout (Element_end) && sizeof (Element_end) == sizeof (Element));

        // Extended Heap Element: Type 7
        struct Element_log20_txt : Heap_alloc
        {
            Element  const  elem        { Element::Type::LOG20_TXT, sizeof (Element_log20_txt) };
            uint32_t const  count       { 1 };
            uint16_t const  alg         { 0xb };                        // SHA256
            uint16_t const  reserved    { 0 };
            uint64_t const  phys;
            uint32_t const  size;
            uint32_t const  off_first   { 0 };
            uint32_t const  off_next    { 0 };

            Element_log20_txt (uint64_t p, uint32_t s) : phys (p), size (s) {}
        };

        static_assert (__is_standard_layout (Element_log20_txt) && sizeof (Element_log20_txt) == sizeof (Element) + 28);

        // Extended Heap Element: Type 8
        struct Element_log20_tcg : Heap_alloc
        {
            Element  const  elem        { Element::Type::LOG20_TCG, sizeof (Element_log20_tcg) };
            uint64_t const  phys;
            uint32_t const  size;
            uint32_t const  off_first   { 0 };
            uint32_t const  off_next    { 0 };

            Element_log20_tcg (uint64_t p, uint32_t s) : phys (p), size (s) {}
        };

        static_assert (__is_standard_layout (Element_log20_tcg) && sizeof (Element_log20_tcg) == sizeof (Element) + 20);

        // Memory Descriptor Record
        struct Mdr
        {
            uint64_t const  phys;                                       // 0
            uint64_t const  size;                                       // 8
            uint8_t  const  type;                                       // 16
            uint8_t  const  rsvd[7];                                    // 17
        };

        static_assert (__is_standard_layout (Mdr) && sizeof (Mdr) == 24);

        // Heap Data Size
        struct Data
        {
            uint64_t        size;                                       // 0

            auto next() const { return reinterpret_cast<uintptr_t>(this) + size; }
        };

        static_assert (__is_standard_layout (Data) && sizeof (Data) == 8);

        // Section C.2: BIOS Data
        struct Data_bios
        {
            Data     const  data;
            uint32_t const  version;                                    // 0    v1+
            uint32_t const  sinit_size;                                 // 4    v1+
            uint64_t const  lcp_pd_base;                                // 8    v2+
            uint64_t const  lcp_pd_size;                                // 16   v2+
            uint32_t const  num_cpu;                                    // 24   v2+
            uint32_t const  sinit_flags;                                // 28   v3+
            uint32_t const  mle_flags;                                  // 32   v5+

            auto plat() const { return version < 6 ? 0 : mle_flags >> 1 & BIT_RANGE (1, 0); }
            auto elem() const { return version < 4 ? nullptr : reinterpret_cast<Element const *>(reinterpret_cast<uintptr_t>(this + 1)); }
        };

        static_assert (__is_standard_layout (Data_bios) && sizeof (Data_bios) == sizeof (Data) + 36);

        // Section C.3: OS to MLE Data
        struct Data_os_mle : Heap_alloc
        {
            Data     const  data;
            uint32_t const  version         { 0 };                      // 0
            uint32_t const  pad             { 0 };                      // 4
            uint64_t        msr_ia32_mtrr_def_type;                     // 8
            uint64_t        msr_ia32_misc_enable;                       // 16
            uint64_t        msr_ia32_debugctl;                          // 24

            struct Mtrr : Heap_alloc
            {
                uint64_t const base;
                uint64_t const mask;

                Mtrr (uint64_t b, uint64_t m) : base (b), mask (m) {}
            };

            Data_os_mle (unsigned c) : data { .size = sizeof (Data_os_mle) + c * sizeof (Mtrr) } {}

            auto mtrr() const { return reinterpret_cast<Mtrr const *>(reinterpret_cast<uintptr_t>(this + 1)); }
        };

        static_assert (__is_standard_layout (Data_os_mle) && sizeof (Data_os_mle) == sizeof (Data) + 32);

        // Section C.4: OS to SINIT Data
        struct Data_os_sinit : Heap_alloc
        {
            Data            data            { sizeof (Data_os_sinit) };
            uint32_t        version         { version_os_sinit };       // 0    v1+
            uint32_t        flags;                                      // 4    v7+
            uint64_t        mle_ptab;                                   // 8    v1+
            uint64_t        mle_size;                                   // 16   v1+
            uint64_t        mle_header;                                 // 24   v1+
            uint64_t        pmr_lo_base;                                // 32   v3+
            uint64_t        pmr_lo_size;                                // 40   v3+
            uint64_t        pmr_hi_base     { 0 };                      // 48   v3+
            uint64_t        pmr_hi_size     { 0 };                      // 56   v3+
            uint64_t        lcp_po_base     { 0 };                      // 64   v3+
            uint64_t        lcp_po_size     { 0 };                      // 72   v3+
            uint32_t        capabilities;                               // 80   v3+
            uint64_t        efi_rsdp;                                   // 84   v6+

            auto elem() const { return version < 6 ? nullptr : reinterpret_cast<Element const *>(reinterpret_cast<uintptr_t>(this + 1)); }
        };

        static_assert (__is_standard_layout (Data_os_sinit) && sizeof (Data_os_sinit) == sizeof (Data) + 92);

        // Section C.5: SINIT to MLE Data
        struct Data_sinit_mle
        {
            Data     const  data;
            uint32_t const  version;                                    // 0    v1+
            uint32_t const  reserved1[29];                              // 4
            uint32_t const  rlp_wakeup;                                 // 120  v5+
            uint32_t const  reserved2;                                  // 124
            uint32_t const  mdrs_count;                                 // 128  v5+
            uint32_t const  mdrs_offset;                                // 132  v5+
            uint32_t const  dmar_size;                                  // 136  v5+
            uint32_t const  dmar_offset;                                // 140  v5+
            uint32_t const  scrtm_status;                               // 144  v8

            auto elem() const { return version < 8 ? nullptr : reinterpret_cast<Element const *>(reinterpret_cast<uintptr_t>(this + 1)); }
        };

        static_assert (__is_standard_layout (Data_sinit_mle) && sizeof (Data_sinit_mle) == sizeof (Data) + 148);

        // Section 2.1: MLE Header
        struct Mle_header
        {
            Uuid     const  uuid;                                       // 0
            uint32_t const  size;                                       // 16
            uint32_t const  version;                                    // 20
            uint32_t const  entry;                                      // 24
            uint32_t const  first;                                      // 28
            uint32_t const  mle_start;                                  // 32
            uint32_t const  mle_end;                                    // 36
            uint32_t        mle_caps;                                   // 40
            uint32_t const  cmd_start;                                  // 44
            uint32_t const  cmd_end;                                    // 48
        };

        static_assert (__is_standard_layout (Mle_header) && sizeof (Mle_header) == 52);

#pragma pack()

        // Section B.1: TXT Registers
        enum class Space : unsigned
        {
            PRIVATE             = 0x00000,
            PUBLIC              = 0x10000,
        };

        enum class Register8 : unsigned
        {
            RESET               = 0x038,        // -- -w System Reset
            PRIVATE_OPEN        = 0x040,        // -- -w Private Space Open
            PRIVATE_CLOSE       = 0x048,        // -- -w Private Space Close
            MEMCFG_UNLOCK       = 0x218,        // -- -w Memory Configuration Unlock
            BASE_LOCK           = 0x230,        // -- -w Base Registers Lock
            BASE_UNLOCK         = 0x238,        // -- -w Base Registers Unlock
            WB_FLUSH            = 0x258,        // -- -w Write Buffer Flush
            LOCALITY1_OPEN      = 0x380,        // -- -w Locality 1 Open
            LOCALITY1_CLOSE     = 0x388,        // -- -w Locality 1 Close
            LOCALITY2_OPEN      = 0x390,        // -- -w Locality 2 Open
            LOCALITY2_CLOSE     = 0x398,        // -- -w Locality 2 Close
            SECRETS_SET         = 0x8e0,        // -- -w Set Secrets
            SECRETS_CLR         = 0x8e8,        // -- -w Clr Secrets
        };

        enum class Register32 : unsigned
        {
            ERRORCODE           = 0x030,        // r- rw Error Code
            VER_FSBIF           = 0x100,        // r- r- Version: Frontside Bus Interface
            VER_QPIIF           = 0x200,        // r- r- Version: Quickpath Interconnect Interface
            NODMA_BASE          = 0x260,        // rw rw NODMA Base
            NODMA_SIZE          = 0x268,        // r- r- NODMA Size
            SINIT_BASE          = 0x270,        // rw rw SINIT Base
            SINIT_SIZE          = 0x278,        // rw rw SINIT Size
            MLE_JOIN            = 0x290,        // rw rw MLE Join Address
            HEAP_BASE           = 0x300,        // rw rw TXT Heap Base
            HEAP_SIZE           = 0x308,        // rw rw TXT Heap Size
            MSEG_BASE           = 0x310,        // rw rw TXT MSEG Base
            MSEG_SIZE           = 0x318,        // rw rw TXT MSEG Size
            DPR                 = 0x330,        // rw rw DMA Protected Range
        };

        enum class Register64 : unsigned
        {
            STS                 = 0x000,        // r- r- Status
            ESTS                = 0x008,        // r- rw Error Status
            THREADS_EXIST       = 0x010,        // r- r- Threads Exist
            THREADS_JOIN        = 0x020,        // r- r- Threads Joined
            ACM_STATUS          = 0x0a0,        // r- rw ACM Status
            DIDVID              = 0x110,        // r- r- Device ID
            ACM_ERRORCODE       = 0x328,        // rw rw ACM Errorcode
            ACM_POLICY_STATUS   = 0x378,        // r- rw ACM Policy Status
            PUBLIC_KEY          = 0x400,        // r- r- ACM Public Key Hash
            DIDVID2             = 0x810,        // r- r- Device ID 2
            E2STS               = 0x8f0,        // r- rw Extended Error Status
        };

        enum STS
        {
            SEQ_IN_PROGRESS     = BIT (17),     // Set between TXT.SEQUENCE.START and TXT.SEQUENCE.DONE
            OPENED_LOCALITY2    = BIT (16),     // Set between TXT.CMD.OPEN.LOCALITY2 and TXT.CMD.CLOSE.LOCALITY2 or between TXT.CMD.OPEN.PRIVATE and TXT.CMD.CLOSE.PRIVATE
            OPENED_LOCALITY1    = BIT (15),     // Set between TXT.CMD.OPEN.LOCALITY1 and TXT.CMD.CLOSE.LOCALITY1
            OPENED_LOCALITY3    = BIT (14),     // Set between TXT.CMD.OPEN.LOCALITY3 and TXT.CMD.CLOSE.LOCALITY3
            OPENED_SMM          = BIT (13),
            LOCKED_PMRC         = BIT (12),
            MEMCFG_OK           = BIT (11),     // Set between TXT.CMD.MEM-CONFIG-CHECKED and TXT.CMD.UNLOCK-MEMCONFIG
            NODMA_TABLE         = BIT (10),     // Set between TXT.CMD.NODMA-TABLE.EN and TXT.CMD.NODMA-TABLE.DIS
            NODMA_CACHE         = BIT  (9),     // Set between TXT.CMD.NODMA-CACHE.EN and TXT.CMD.NODMA-CACHE.DIS
            OPENED_PRIVATE      = BIT  (7),     // Set between TXT.CMD.OPEN-PRIVATE and TXT.CMD.CLOSE-PRIVATE
            LOCKED_MEMCFG       = BIT  (6),     // Clr by TXT.CMD.UNLOCK.MEMCONFIG (VTBAR/VTCTRL locked)
            LOCKED_BASE         = BIT  (5),     // Set between TXT.CMD.LOCK-BASE and TXT.CMD.UNLOCK-BASE (HEAP_BASE/HEAP_SIZE, MSEG_BASE/MSEG_SIZE, Scratchpad locked)
            UNLOCKED_MEM        = BIT  (4),     // Set by TXT.CMD.UNLOCK-MEMORY
            DONE_SEXIT          = BIT  (1),     // THREADS_JOIN == 0
            DONE_SENTER         = BIT  (0),     // THREADS_JOIN != 0 && THREADS_JOIN == THREADS_EXIST
        };

        enum ESTS
        {
            WAKE_ERROR          = BIT  (6),     // Reset or power failure with secrets in memory
            ALIAS_FAULT         = BIT  (5),     // Alias Error Violation
            MEMORY_ATTACK       = BIT  (2),     // Illegal read of DRAM
            ROGUE               = BIT  (1),     // CPU has left the secure environment improperly
            POISON              = BIT  (0),     // TXT.POISON cycle received
        };

        enum E2STS
        {
            SECRETS             = BIT  (1),     // Secrets in memory
        };

        enum VER_QPIIF
        {
            PRD                 = BIT (31),     // Production Fused
            TXT                 = BIT (27),     // TXT Capable
            DPR                 = BIT (26),     // DPR Capable
            PMRC                = BIT (19),     // PMRC Capable
        };

        static auto read  (Space s, Register32 r)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TXTC | std::to_underlying (s) | std::to_underlying (r)); }
        static auto read  (Space s, Register64 r)      { return *reinterpret_cast<uint64_t volatile *>(MMAP_GLB_TXTC | std::to_underlying (s) | std::to_underlying (r)); }
        static void write (Space s, Register32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TXTC | std::to_underlying (s) | std::to_underlying (r)) = v; }
        static void write (Space s, Register64 r, uint64_t v) { *reinterpret_cast<uint64_t volatile *>(MMAP_GLB_TXTC | std::to_underlying (s) | std::to_underlying (r)) = v; }

        [[nodiscard]] static bool command (Register8 c, Register64 s, uint64_t f)
        {
            /*
             * Command registers are write-only in the TXT private configuration space.
             * Accesses to command registers are done with 1-byte writes.
             * The data bits associated with a command are undefined and have no specific meaning.
             */
            *reinterpret_cast<uint8_t volatile *>(MMAP_GLB_TXTC + std::to_underlying (Space::PRIVATE) + std::to_underlying (c)) = 0;

            /*
             * After writing to a command register, software should read the
             * corresponding status flag for that command to ensure that the
             * command has completed successfully.
             */
            return read (Space::PUBLIC, s) & f;
        };

        ALWAYS_INLINE static inline bool check_acm (uint32_t, uint32_t, uint32_t, uint32_t &, uint32_t &, uint32_t &);
        ALWAYS_INLINE static inline bool init_heap (uint32_t, uint32_t, uint32_t, uint32_t, unsigned);
        ALWAYS_INLINE static inline bool init_mtrr (uint64_t, uint64_t, unsigned, unsigned);

        static void parse_elem (Element const *, uintptr_t, uintptr_t);

        static void restore() asm ("txt_restore");

    public:
        static inline bool launched asm ("launched") { false };

        static void launch();

        static void init();
        static void fini();
};
