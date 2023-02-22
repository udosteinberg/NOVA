/*
 * Trusted Execution Technology (TXT)
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

#include "byteorder.hpp"
#include "compiler.hpp"
#include "memory.hpp"
#include "std.hpp"
#include "uuid.hpp"

class Txt final
{
    private:
        static constexpr auto ver_pre_acm { 7 };

        /*
         * 0xfed20000-0xfed2ffff:  64K TXT private
         * 0xfed30000-0xfed3ffff:  64K TXT public
         * 0xfed40000-0xfed44fff:  20K TPM localities
         * 0xfed45000-0xfed7ffff: 236K TXT reserved
         */
        static constexpr auto txt_base { 0xfed20000 };
        static constexpr auto txt_size { 0x60000 };

        /*
         * Section C.1: Extended Heap Element
         */
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

            Unaligned_le<uint32_t>    const type;
            Unaligned_le<uint32_t>    const size;

            auto get_type() const { return Type { uint32_t { type } }; }
            auto get_data() const { return reinterpret_cast<uintptr_t>(this + 1); }
            auto get_next() const { return reinterpret_cast<Element const *>(reinterpret_cast<uintptr_t>(this) + size); }

            explicit Element (Type t, uint32_t s) : type { std::to_underlying (t) }, size { s } {}
        };

        static_assert (__is_standard_layout (Element) && alignof (Element) == 1 && sizeof (Element) == 8);

        /*
         * Extended Heap Element: Type 0
         */
        struct Element_end
        {
            Element                   const elem        { Element::Type::END, sizeof (Element_end) };
        };

        static_assert (__is_standard_layout (Element_end) && alignof (Element_end) == 1 && sizeof (Element_end) == sizeof (Element));

        /*
         * Extended Heap Element: Type 7
         */
        struct Element_log20_txt
        {
            Element                   const elem        { Element::Type::LOG20_TXT, sizeof (Element_log20_txt) };
            Unaligned_le<uint32_t>    const count       { 1 };
            Unaligned_le<uint16_t>    const alg         { 0xb };                        // SHA256
            Unaligned_le<uint16_t>    const reserved    { 0 };
            Unaligned_le<uint64_t>    const phys;
            Unaligned_le<uint32_t>    const size;
            Unaligned_le<uint32_t>    const off_first   { 0 };
            Unaligned_le<uint32_t>    const off_next    { 0 };

            explicit Element_log20_txt (uint64_t p, uint32_t s) : phys { p }, size { s } {}
        };

        static_assert (__is_standard_layout (Element_log20_txt) && alignof (Element_log20_txt) == 1 && sizeof (Element_log20_txt) == sizeof (Element) + 28);

        /*
         * Extended Heap Element: Type 8
         */
        struct Element_log20_tcg
        {
            Element                   const elem        { Element::Type::LOG20_TCG, sizeof (Element_log20_tcg) };
            Unaligned_le<uint64_t>    const phys;
            Unaligned_le<uint32_t>    const size;
            Unaligned_le<uint32_t>    const off_first   { 0 };
            Unaligned_le<uint32_t>    const off_next    { 0 };

            explicit Element_log20_tcg (uint64_t p, uint32_t s) : phys { p }, size { s } {}
        };

        static_assert (__is_standard_layout (Element_log20_tcg) && alignof (Element_log20_tcg) == 1 && sizeof (Element_log20_tcg) == sizeof (Element) + 20);

        /*
         * Heap Data Size
         */
        struct Data
        {
            Unaligned_le<uint64_t>          size;                                       // 0    v1+

            auto next() const { return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(this) + size); }
        };

        static_assert (__is_standard_layout (Data) && alignof (Data) == 1 && sizeof (Data) == 8);

        /*
         * Section C.2: EFI to PRE Data
         */
        struct Data_efi_pre
        {
            Data                      const data;
            Unaligned_le<uint32_t>    const version;                                    // 0    v1+
            Unaligned_le<uint32_t>    const sinit_size;                                 // 4    v1+
            Unaligned_le<uint64_t>    const lcp_pd_base;                                // 8    v2+
            Unaligned_le<uint64_t>    const lcp_pd_size;                                // 16   v2+
            Unaligned_le<uint32_t>    const num_cpu;                                    // 24   v2+
            Unaligned_le<uint32_t>    const sinit_flags;                                // 28   v3+
            Unaligned_le<uint32_t>    const mle_flags;                                  // 32   v5+

            auto plat() const { return version < 6 ? 0 : mle_flags >> 1 & BIT_RANGE (1, 0); }

            auto elem() const { return version < 4 ? nullptr : reinterpret_cast<Element const *>(this + 1); }
        };

        static_assert (__is_standard_layout (Data_efi_pre) && alignof (Data_efi_pre) == 1 && sizeof (Data_efi_pre) == sizeof (Data) + 36);

        /*
         * Section C.3: PRE to MLE Data
         */
        struct Data_pre_mle
        {
            Data                      const data;
            Unaligned_le<uint64_t>    const ia32_mtrr_def_type;                         // 0
            Unaligned_le<uint64_t>    const ia32_misc_enable;                           // 8
            Unaligned_le<uint64_t>    const ia32_debugctl;                              // 16

            struct Mtrr_backup
            {
                Unaligned_le<uint64_t> const base;
                Unaligned_le<uint64_t> const mask;
            };

            auto mtrr()       { return reinterpret_cast<Mtrr_backup       *>(this + 1); }
            auto mtrr() const { return reinterpret_cast<Mtrr_backup const *>(this + 1); }
        };

        static_assert (__is_standard_layout (Data_pre_mle) && alignof (Data_pre_mle) == 1 && sizeof (Data_pre_mle) == sizeof (Data) + 24);

        /*
         * Section C.4: PRE to ACM Data
         */
        struct Data_pre_acm
        {
            Data                            data            { sizeof (Data_pre_acm) };
            Unaligned_le<uint32_t>    const version         { ver_pre_acm };            // 0    v1+
            Unaligned_le<uint32_t>    const flags;                                      // 4    v7+
            Unaligned_le<uint64_t>    const mle_ptab;                                   // 8    v1+
            Unaligned_le<uint64_t>    const mle_size;                                   // 16   v1+
            Unaligned_le<uint64_t>    const mle_header;                                 // 24   v1+
            Unaligned_le<uint64_t>    const pmr_lo_base;                                // 32   v3+
            Unaligned_le<uint64_t>    const pmr_lo_size;                                // 40   v3+
            Unaligned_le<uint64_t>    const pmr_hi_base     { 0 };                      // 48   v3+
            Unaligned_le<uint64_t>    const pmr_hi_size     { 0 };                      // 56   v3+
            Unaligned_le<uint64_t>    const lcp_po_base     { 0 };                      // 64   v3+
            Unaligned_le<uint64_t>    const lcp_po_size     { 0 };                      // 72   v3+
            Unaligned_le<uint32_t>    const caps;                                       // 80   v3+
            Unaligned_le<uint64_t>    const rsdp;                                       // 84   v6+

            void append (Element const &e) { data.size = data.size + e.size; }

            auto elem() const { return version < 6 ? nullptr : reinterpret_cast<Element const *>(this + 1); }
        };

        static_assert (__is_standard_layout (Data_pre_acm) && alignof (Data_pre_acm) == 1 && sizeof (Data_pre_acm) == sizeof (Data) + 92);

        /*
         * Section C.5: ACM to MLE Data
         */
        struct Data_acm_mle
        {
            Data                      const data;
            Unaligned_le<uint32_t>    const version;                                    // 0    v1+
            Unaligned_le<uint32_t>    const reserved1[29];                              // 4
            Unaligned_le<uint32_t>    const rlp_wakeup;                                 // 120  v5+
            Unaligned_le<uint32_t>    const reserved2;                                  // 124
            Unaligned_le<uint32_t>    const mdrs_count;                                 // 128  v5+
            Unaligned_le<uint32_t>    const mdrs_offset;                                // 132  v5+
            Unaligned_le<uint32_t>    const dmar_size;                                  // 136  v5+
            Unaligned_le<uint32_t>    const dmar_offset;                                // 140  v5+
            Unaligned_le<uint32_t>    const scrtm_status;                               // 144  v8

            auto elem() const { return version < 8 ? nullptr : reinterpret_cast<Element const *>(this + 1); }
        };

        static_assert (__is_standard_layout (Data_acm_mle) && alignof (Data_acm_mle) == 1 && sizeof (Data_acm_mle) == sizeof (Data) + 148);

        /*
         * Section 2.1: MLE Header
         */
        struct Mle_header
        {
            Unaligned_le<Uuid>        const uuid;                                       // 0
            Unaligned_le<uint32_t>    const size;                                       // 16
            Unaligned_le<uint32_t>    const version;                                    // 20
            Unaligned_le<uint32_t>    const entry;                                      // 24
            Unaligned_le<uint32_t>    const first;                                      // 28
            Unaligned_le<uint32_t>    const mle_start;                                  // 32
            Unaligned_le<uint32_t>    const mle_end;                                    // 36
            Unaligned_le<uint32_t>          mle_caps;                                   // 40
            Unaligned_le<uint32_t>    const cmd_start;                                  // 44
            Unaligned_le<uint32_t>    const cmd_end;                                    // 48
        };

        static_assert (__is_standard_layout (Mle_header) && alignof (Mle_header) == 1 && sizeof (Mle_header) == 52);

        // Section B.1: TXT Registers
        enum class Space : unsigned
        {
            PRIVATE             = 0x00000,
            PUBLIC              = 0x10000,
        };

        enum class Reg8 : unsigned
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

        enum class Reg32 : unsigned
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

        enum class Reg64 : unsigned
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
            LOCALITY2           = BIT (16),     // Set between TXT.CMD.OPEN.LOCALITY2 and TXT.CMD.CLOSE.LOCALITY2 or between TXT.CMD.OPEN.PRIVATE and TXT.CMD.CLOSE.PRIVATE
            LOCALITY1           = BIT (15),     // Set between TXT.CMD.OPEN.LOCALITY1 and TXT.CMD.CLOSE.LOCALITY1
            LOCALITY3           = BIT (14),     // Set between TXT.CMD.OPEN.LOCALITY3 and TXT.CMD.CLOSE.LOCALITY3
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

        static auto read  (Space s, Reg8  r)      { return *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TXTC + std::to_underlying (s) + std::to_underlying (r)); }
        static auto read  (Space s, Reg32 r)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TXTC + std::to_underlying (s) + std::to_underlying (r)); }
        static auto read  (Space s, Reg64 r)      { return *reinterpret_cast<uint64_t volatile *>(MMAP_GLB_TXTC + std::to_underlying (s) + std::to_underlying (r)); }

        static void write (Space s, Reg8  r, uint8_t  v) { *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TXTC + std::to_underlying (s) + std::to_underlying (r)) = v; }
        static void write (Space s, Reg32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TXTC + std::to_underlying (s) + std::to_underlying (r)) = v; }
        static void write (Space s, Reg64 r, uint64_t v) { *reinterpret_cast<uint64_t volatile *>(MMAP_GLB_TXTC + std::to_underlying (s) + std::to_underlying (r)) = v; }

        [[nodiscard]] static bool command (Reg8 c, Reg64 s, uint64_t f)
        {
            /*
             * Command registers are write-only in the TXT private configuration space.
             * Accesses to command registers are done with 1-byte writes.
             * The data bits associated with a command are undefined and have no specific meaning.
             */
            write (Space::PRIVATE, c, 0);

            /*
             * After writing to a command register, software should read the
             * corresponding status flag for that command to ensure that the
             * command has completed successfully.
             */
            return read (Space::PUBLIC, s) & f;
        }

        ALWAYS_INLINE static inline bool check_acm (Mle_header *, uint32_t, uint32_t, uint32_t, uint32_t &, uint32_t &, uint32_t &);
        ALWAYS_INLINE static inline bool init_heap (Mle_header *, uint32_t, uint32_t, uint32_t, uint32_t, unsigned);
        ALWAYS_INLINE static inline bool init_mtrr (uint64_t, uint64_t, unsigned, unsigned);

        static void parse_elem (Element const *, void const *, uintptr_t);

        static void restore() asm ("txt_restore");

    public:
        static inline constinit bool launched asm ("launched") { false };

        static void launch();

        static void init();
        static void fini();
};
