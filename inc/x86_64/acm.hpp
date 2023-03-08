/*
 * Authenticated Code Module (ACM)
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

#include "endian.hpp"
#include "macros.hpp"
#include "uuid.hpp"

class Acm final
{
    public:
        // ACM/MLE Capabilities
        enum Cap
        {
            WAKEUP_GETSEC       = BIT  (0),     // Supports RLP wakeup via GETSEC[WAKEUP]
            WAKEUP_MONITOR      = BIT  (1),     // Supports RLP wakeup via MONITOR
            MLE_PTAB_PTR        = BIT  (2),     // Supports MLE PTAB pointer in ECX at entry
            STM                 = BIT  (3),     // Supports SMI Transfer Monitor
            TPM_12_PCR_NL       = BIT  (4),     // Supports PCR non-legacy usage
            TPM_12_PCR_DA       = BIT  (5),     // Supports PCR details/authorities
            PLAT_CLIENT         = BIT  (6),     // Supports client platforms
            PLAT_SERVER         = BIT  (7),     // Supports server platforms
            MAXPHYADDR          = BIT  (8),     // Supports all physical bits in MTRRs
            TPM_20_TCG_LOG      = BIT  (9),     // Supports TCG format in TPM 2.0 event log
            CBNT                = BIT (10),     // Supports Converged BtG/TXT
            MPD                 = BIT (11),     // Supports Memory Power Down
            TPR                 = BIT (14),     // Supports TPR DMA Protection
        };

        // TPM Capabilities
        enum Tpm
        {
            MAX_AGILITY         = BIT  (0),     // Supports maximum agility policy
            MAX_PERFORMANCE     = BIT  (1),     // Supports maximum performance policy
            DTPM_12             = BIT  (2),     // Supports discrete TPM 1.2
            DTPM_20             = BIT  (3),     // Supports discrete TPM 2.0
            FTPM_20             = BIT  (5),     // Supports firmware TPM 2.0
            TPM_20_TCG_NV       = BIT  (6),     // Supports TCG NV indexes in TPM 2.0
        };

        // PCH ID
        struct Id_pch
        {
            Unaligned_le<uint32_t> const flags;
            Unaligned_le<uint16_t> const vid, did, rid, res[3];

            bool match (unsigned count, uint64_t didvid) const
            {
                auto const v { static_cast<uint16_t>(didvid) };
                auto const d { static_cast<uint16_t>(didvid >> 16) };
                auto const r { static_cast<uint16_t>(didvid >> 32) };

                for (auto pch { this }; count--; pch++)
                    if (pch->vid == v && pch->did == d && (pch->flags & BIT (0) ? pch->rid & r : pch->rid == r))
                        return true;

                return false;
            }
        };

        static_assert (__is_standard_layout (Id_pch) && alignof (Id_pch) == 1 && sizeof (Id_pch) == 16);

        // CPU ID
        struct Id_cpu
        {
            Unaligned_le<uint32_t> const fms, fms_msk;
            Unaligned_le<uint64_t> const pid, pid_msk;

            bool match (unsigned count, uint32_t f, uint64_t p) const
            {
                for (auto cpu { this }; count--; cpu++)
                    if (cpu->fms == (cpu->fms_msk & f) && cpu->pid == (cpu->pid_msk & p))
                        return true;

                return false;
            }
        };

        static_assert (__is_standard_layout (Id_cpu) && alignof (Id_cpu) == 1 && sizeof (Id_cpu) == 24);

        // ID List
        struct List
        {
            Unaligned_le<uint32_t> const count;

            auto list() const { return reinterpret_cast<uintptr_t>(this + 1); }
        };

        static_assert (__is_standard_layout (List) && alignof (List) == 1 && sizeof (List) == 4);

        // ACM Information Table
        struct Info
        {
            Uuid        uuid;                               // v2+
            uint8_t     acm_type;                           // v2+
            uint8_t     version;                            // v2+
            uint16_t    length;                             // v2+
            uint32_t    pch_list;                           // v2+
            uint32_t    max_ver_pre_acm;                    // v2+
            uint32_t    min_ver_mle_hdr;                    // v2+
            uint32_t    capabilities;                       // v3+
            uint8_t     acm_version;                        // v3+
            uint8_t     acm_major, acm_minor, acm_build;    // v6+
            uint32_t    cpu_list;                           // v4+
            uint32_t    tpm_list;                           // v5+
        };

        static_assert (__is_standard_layout (Info) && alignof (Info) == 8 && sizeof (Info) == 48);

        // ACM Header
        struct Header
        {
            uint16_t    type;                               // Chipset (2)
            uint16_t    subtype;                            // SINIT (0), Startup (1), BootGuard (3)
            uint32_t    header_size, header_version;
            uint16_t    chipset, flags;
            uint32_t    vendor, date, total_size;
            uint16_t    txt_svn, sgx_svn;
            uint32_t    code_ctrl, error_entry, gdt_limit, gdt_base, sel, eip;
            uint64_t    reserved2[8];
            uint32_t    key_size, scratch_size;
        };

        static_assert (__is_standard_layout (Header) && alignof (Header) == 8 && sizeof (Header) == 128);
};
