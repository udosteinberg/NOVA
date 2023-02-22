/*
 * Trusted Execution Technology (TXT)
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

#include "acm.hpp"
#include "acpi_table.hpp"
#include "cache_guard.hpp"
#include "mtrr.hpp"
#include "multiboot.hpp"
#include "smx.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"
#include "tpm.hpp"
#include "tpm_log.hpp"
#include "txt.hpp"
#include "uefi.hpp"

bool Txt::check_acm (Mle_header const *hdr, uintptr_t ptr, size_t avl, uint32_t fms, uint32_t &acm_size, uint32_t &acm_caps, uint32_t &tpm_caps)
{
    // ACM header must fit and have correct type
    auto const acm { view_buffer_as<Acm::Header const> (ptr, avl, 0) };
    if (!acm || acm->type != 2 || acm->subtype != 0) [[unlikely]]
        return false;

    // ACM PRD/DBG signing must match chipset PRD/DBG fusing
    if (!(acm->flags & BIT (15)) == !(read (Space::PUBLIC, Reg32::VER_QPIIF) & VER_QPIIF::PRD)) [[unlikely]]
        return false;

    // ACM must fit into the SINIT region
    if (acm->total_size > avl / 4) [[unlikely]]
        return false;

    // ACM must have a reasonable size
    uint32_t const size { acm->total_size * 4 };
    if (size < sizeof (Acm::Header))
        return false;

    // Information table must be at a 64-byte aligned offset
    auto const offs { 4 * (size_t { acm->header_size } + size_t { acm->scratch_size }) };
    if (offs % 64) [[unlikely]]
        return false;

    // Information table must fully reside within ACM, version >= 5 and ACM type/length/UUID must be valid
    auto const info { view_buffer_as<Acm::Info const> (ptr, size, offs) };
    if (!info || info->acm_type != 1 || info->version < 5 || info->length < sizeof (Acm::Info) || info->uuid != Uuid { 0x7fc03aaa, 0x46a7, 0x18db, { 0x2e, 0xac, 0x69, 0x8f, 0x8d, 0x41, 0x7f, 0x5a }}) [[unlikely]]
        return false;

    // ACM must support PRE-ACM version, MLE header version and TCG Event Log format
    if (info->max_ver_pre_acm < ver_pre_acm || info->min_ver_mle_hdr > hdr->version || !(info->capabilities & Acm::Cap::TPM_20_TCG_LOG)) [[unlikely]]
        return false;

    // PCH list must fully reside within ACM and PCH must be supported (v2+)
    auto const pch { view_buffer_as<Acm::List const> (ptr, size, info->pch_list) };
    auto const pch_id { pch ? view_buffer_as<Acm::Id_pch const> (ptr, size, info->pch_list + sizeof (Acm::List), pch->count) : nullptr };
    if (!pch_id || !pch_id->match (pch->count, read (Space::PUBLIC, Reg64::DIDVID))) [[unlikely]]
        return false;

    // CPU list must fully reside within ACM and CPU must be supported (v4+)
    auto const cpu { view_buffer_as<Acm::List const> (ptr, size, info->cpu_list) };
    auto const cpu_id { cpu ? view_buffer_as<Acm::Id_cpu const> (ptr, size, info->cpu_list + sizeof (Acm::List), cpu->count) : nullptr };
    if (!cpu_id || !cpu_id->match (cpu->count, fms, Msr::read (Msr::Reg64::IA32_PLATFORM_ID))) [[unlikely]]
        return false;

    // TPM list must fully reside within ACM (v5+)
    auto const tpm { view_buffer_as<Unaligned_le<uint32_t> const> (ptr, size, info->tpm_list) };

    acm_size = size;
    acm_caps = info->capabilities;
    tpm_caps = tpm ? uint32_t { *tpm } : 0;

    return true;
}

bool Txt::init_heap (Mle_header const *hdr, uintptr_t ptr, size_t avl, uint32_t acm_caps, uint32_t tpm_caps, unsigned vcnt)
{
    // EFI_PRE was constructed by firmware
    auto const efi_pre { consume<Data_efi_pre> (ptr, avl) };
    if (!efi_pre) [[unlikely]]
        return false;

    // Number of CPUs must be in range [1, NUM_CPU]
    if (!efi_pre->num_cpu || efi_pre->num_cpu > NUM_CPU) [[unlikely]]
        return false;

    // Platform type must be supported by SINIT
    auto const plat { efi_pre->plat() };
    if ((plat == 1 && !(acm_caps & Acm::Cap::PLAT_CLIENT)) || (plat == 2 && !(acm_caps & Acm::Cap::PLAT_SERVER))) [[unlikely]]
        return false;

    // Check if PRE_MLE (including MTRRs) fits
    auto const pre_mle_s { sizeof (Data_pre_mle) + vcnt * sizeof (Data_pre_mle::Mtrr_backup) };
    auto const pre_mle_p { reinterpret_cast<char *>(ptr) };
    if (!reserve (ptr, avl, pre_mle_s)) [[unlikely]]
        return false;

    // Construct PRE_MLE
    auto const pre_mle { new (pre_mle_p) Data_pre_mle
    {
        .size               { pre_mle_s },
        .ia32_mtrr_def_type { Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) },
        .ia32_misc_enable   { Msr::read (Msr::Reg64::IA32_MISC_ENABLE) },
        .ia32_debugctl      { Msr::read (Msr::Reg64::IA32_DEBUGCTL) },
    } };

    // Save MTRRs
    auto mtrr { pre_mle->mtrr() };
    for (unsigned i { 0 }; i < vcnt; i++, mtrr++)
        new (mtrr) Data_pre_mle::Mtrr_backup { Mtrr::get_base (i), Mtrr::get_mask (i) };

    auto const pre_acm_s { sizeof (Data_pre_acm) + sizeof (Element_log20_tcg) + sizeof (Element_end) };
    auto const pre_acm_p { reinterpret_cast<char *>(ptr) };
    if (!reserve (ptr, avl, pre_acm_s)) [[unlikely]]
        return false;

    // Construct PRE_ACM
    new (pre_acm_p) Data_pre_acm
    {
        .size               { pre_acm_s },
        .flags              { !!(tpm_caps & Acm::Tpm::MAX_PERFORMANCE) },
        .mle_ptab           { Kmem::sym_to_phys (&MLE_L2) },
        .mle_size           { hdr->mle_end - hdr->mle_start },
        .mle_header         { reinterpret_cast<uintptr_t>(&HASH_HEAD) },
        .pmr_lo_base        { Kmem::sym_to_phys (&NOVA_HPAS) },
        .pmr_lo_size        { Multiboot::ea - Kmem::sym_to_phys (&NOVA_HPAS) },
        .caps               { hdr->mle_caps & acm_caps },
        .rsdp               { Uefi::info.tbl.rsdp },
    };

    // Add LOG element
    new (pre_acm_p + sizeof (Data_pre_acm)) Element_log20_tcg { Tpm_log::phys(), Tpm_log::size };

    // Add END element
    new (pre_acm_p + sizeof (Data_pre_acm) + sizeof (Element_log20_tcg)) Element_end;

    return true;
}

bool Txt::init_mtrr (uint64_t phys, uint64_t size, unsigned vcnt, unsigned bits)
{
    // Ensure size is a multiple of PAGE_SIZE
    size = aligned_up (PAGE_SIZE (0), size);

    auto const mask { BIT64 (bits) - 1 };

    // Determine the number of variable MTRRs required to cover the region
    unsigned cnt { 0 };
    for (uint64_t p { phys }, s { size }, b; s; s -= b, p += b, cnt++)
        b = BIT64 (aligned_order (s, p));

    // Fail if the region requires more variable MTRRs than available
    if (cnt > vcnt) [[unlikely]]
        return false;

    {   Cache_guard guard;

        // Disable all MTRRs and set default memory type as UC
        Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, CA_TYPE_MEM_UC);

        // Update variable MTRRs to map SINIT ACM as WB
        for (unsigned i { 0 }; i < vcnt; i++) {

            uint64_t b { 0 }, m { 0 };

            if (size) {
                auto const s { BIT64 (aligned_order (size, phys)) };
                b = phys | CA_TYPE_MEM_WB;
                m = (mask & ~(s - 1)) | BIT (11);
                size -= s;
                phys += s;
            }

            Mtrr::set_base (i, b);
            Mtrr::set_mask (i, m);
        }

        // Enable variable MTRRs and set default memory type as UC
        Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, CA_TYPE_MEM_UC | BIT (11));
    }

    return true;
}

void Txt::launch()
{
    uint32_t fms, ebx, ecx, edx, acm_size, acm_caps, tpm_caps;

    Cpu::cpuid (0x1, fms, ebx, ecx, edx);

    // Abort if SMX is not supported
    if (!(ecx & BIT (6))) [[unlikely]]
        return;

    // Enable SMX
    Cr::set_cr4 (Cr::get_cr4() | CR4_SMXE);

    // Abort if SMX capabilities are missing
    if ((Smx::capabilities() & Smx::required) != Smx::required) [[unlikely]]
        return;

    // Map TXT registers
    Ptab_hst::map (MMAP_GLB_TXTC, txt_base, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev(), 1);

    // Determine HEAP base/size
    auto const heap_base { read (Space::PUBLIC, Reg32::HEAP_BASE) };
    auto const heap_size { read (Space::PUBLIC, Reg32::HEAP_SIZE) };

    // Abort if HEAP region is invalid
    if (!heap_base || !heap_size) [[unlikely]]
        return;

    // Map HEAP writable
    auto const heap { reinterpret_cast<uintptr_t>(Ptab_hst::map (MMAP_GLB_TXTH, heap_base, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram(), 2)) };

    // The rest of the function is pre-launch only
    if (launched)
        return;

    // Determine SINIT base/size
    auto const sinit_base { read (Space::PUBLIC, Reg32::SINIT_BASE) };
    auto const sinit_size { read (Space::PUBLIC, Reg32::SINIT_SIZE) };

    // Abort if SINIT region is invalid or not 4K-aligned, which would cause GETSEC[SENTER] to fail
    if (!sinit_base || !sinit_size || sinit_base & OFFS_MASK (0)) [[unlikely]]
        return;

    // Abort if TXT_RESET.STS is set, which would cause GETSEC[SENTER] to fail
    auto const ests { read (Space::PUBLIC, Reg8::ESTS) };
    if (ests & BIT (0)) [[unlikely]]
        return;

    // Abort if TXT errorcode is valid and not an ACM-generated class-0 progress code
    auto const errorcode { read (Space::PUBLIC, Reg32::TXT_ERRORCODE) };
    if (errorcode & BIT (31) && (errorcode & 0x400083f0) != 0x40000000) [[unlikely]]
        return;

    // Abort if any MCE banks report errors
    auto const banks { static_cast<uint8_t>(Msr::read (Msr::Reg64::IA32_MCG_CAP)) };
    for (unsigned i { 0 }; i < banks; i++)
        if (Msr::read (Msr::Arr64::IA32_MC_STATUS, 4, i) & BIT64 (63)) [[unlikely]]
            return;

    // Abort if MCE is in progress
    if (Msr::read (Msr::Reg64::IA32_MCG_STATUS) & BIT (2)) [[unlikely]]
        return;

    // Abort if TPM initialization fails
    if (!Tpm::init (false)) [[unlikely]]
        return;

    auto const mle { std::start_lifetime_as<Mle_header> (Kmem::sym_to_virt (&__head_mle)) };

    // Map SINIT read-only
    auto const sinit { reinterpret_cast<uintptr_t>(Ptab_hst::map (MMAP_GLB_MAP0, sinit_base)) };

    // Abort if SINIT ACM check fails
    if (!check_acm (mle, sinit, min (Pte_hst::page_size (Pte_hst::bpl), sinit_size), fms, acm_size, acm_caps, tpm_caps)) [[unlikely]]
        return;

    // Determine number of variable MTRRs
    auto const vcnt { Mtrr::get_vcnt() };

    // Abort if TXT heap initialization fails
    if (!init_heap (mle, heap, min (Pte_hst::page_size (Pte_hst::bpl), heap_size), acm_caps, tpm_caps, vcnt)) [[unlikely]]
        return;

    // Abort if MTRR initialization fails
    if (!init_mtrr (sinit_base, acm_size, vcnt, acm_caps & Acm::Cap::MAXPHYADDR ? Memattr::kbits + Memattr::obits : 36)) [[unlikely]]
        return;

    // Enter measured launch environment
    Smx::senter (sinit_base, acm_size);
}

void Txt::restore()
{
    // Variable MTRR masks must not set any key bits
    auto const mask { BIT64 (Memattr::obits) - 1 };

    // Determine number of variable MTRRs
    auto const vcnt { Mtrr::get_vcnt() };

    if (launched) [[likely]] {

        // TXT registers and heap have already been mapped by launch()
        auto ptr { MMAP_GLB_TXTH + (read (Space::PUBLIC, Reg32::HEAP_BASE) & Pte_hst::offs_mask (Pte_hst::bpl)) };
        auto avl { size_t { min (Pte_hst::page_size (Pte_hst::bpl), read (Space::PUBLIC, Reg32::HEAP_SIZE)) } };

        auto const efi_pre { consume<Data_efi_pre> (ptr, avl) };
        auto const pre_mle { consume<Data_pre_mle> (ptr, avl, vcnt * sizeof (Data_pre_mle::Mtrr_backup)) };

        if (!efi_pre || !pre_mle) [[unlikely]]
            panic ("DRTM: Malformed TXT heap");

        auto mtrr { pre_mle->mtrr() };

        {   Cache_guard guard;

            // Restore variable MTRRs and zap key bits
            for (unsigned i { 0 }; i < vcnt; i++, mtrr++) {
                Mtrr::set_base (i, mtrr->base);
                Mtrr::set_mask (i, mtrr->mask & mask);
            }

            // Restore MSRs
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE,  pre_mle->ia32_mtrr_def_type);
            Msr::write (Msr::Reg64::IA32_MISC_ENABLE,    pre_mle->ia32_misc_enable);
            Msr::write (Msr::Reg64::IA32_DEBUGCTL,       pre_mle->ia32_debugctl);
        }

        // Enable SMX
        Cr::set_cr4 (Cr::get_cr4() | CR4_SMXE);

        // Reenable SMI
        Smx::smctrl();

    } else {

        // Check for buggy firmware that misprogrammed the MTRRs
        if (!Mtrr::validate (vcnt, ~mask)) [[unlikely]] {

            Cache_guard guard;

            // Disable MTRRs
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) & ~BIT64 (11));

            // Fix up variable MTRRs and zap key bits
            for (unsigned i { 0 }; i < vcnt; i++)
                Mtrr::set_mask (i, Mtrr::get_mask (i) & mask);

            // Enable MTRRs
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) | BIT64 (11));
        }
    }
}

void Txt::parse_elem (uintptr_t ptr, size_t avl, uintptr_t o, uint32_t m)
{
    // Iterate Extended Heap Elements
    for (Element const *e; (e = consume<Element> (ptr, avl)); ) {

        auto const type { e->get_type() };

        // Ignore element types the mask cannot express or does not select
        if (std::to_underlying (type) > type_msbn<decltype (m)>() || !(m & BIT (std::to_underlying (type)))) [[unlikely]]
            continue;

        switch (type) {

            default:
                break;

            case Element::Type::END:
                return;

            // Override these ACPI tables with their ACM-validated copies on the TXT heap
            case Element::Type::ACPI_MADT:
            case Element::Type::ACPI_MCFG:
            case Element::Type::ACPI_DTPR:
            case Element::Type::ACPI_CEDT:
                {
                    auto const tbl { reinterpret_cast<uintptr_t>(e + 1) };
                    std::start_lifetime_as<Acpi_table const> (tbl)->validate (tbl - o, e->size - sizeof (*e));
                }
                break;

            case Element::Type::LOG20_TCG:
                {
                    auto const log { std::start_lifetime_as<Element_log20_tcg const> (e) };
                    Tpm_log::init (log->phys, log->size, log->off_next);
                }
                break;
        }
    }
}

void Txt::init()
{
    // Abort if there was no measured launch
    if (!launched) [[unlikely]]
        return;

    if (!Tpm::init (true)) [[unlikely]]
        panic ("DRTM: TPM initialization failed");

    auto const dv1 { read (Space::PUBLIC, Reg64::DIDVID) };
    auto const dv2 { read (Space::PUBLIC, Reg64::DIDVID2) };

    trace (TRACE_DRTM, "DRTM: %04x:%04x (%#x) %04x:%04x",
           static_cast<uint16_t>(dv1), static_cast<uint16_t>(dv1 >> 16), static_cast<uint16_t>(dv1 >> 32),
           static_cast<uint16_t>(dv2), static_cast<uint16_t>(dv2 >> 16));

    auto const sinit_base { read (Space::PUBLIC, Reg32::SINIT_BASE) };
    auto const sinit_size { read (Space::PUBLIC, Reg32::SINIT_SIZE) };

    auto const heap_base { read (Space::PUBLIC, Reg32::HEAP_BASE) };
    auto const heap_size { read (Space::PUBLIC, Reg32::HEAP_SIZE) };

    trace (TRACE_DRTM, "DRTM: SINIT:%#x/%#x HEAP:%#x/%#x", sinit_base, sinit_size, heap_base, heap_size);

    // Deny user access to SINIT region
    Space_hst::access_ctrl (sinit_base, sinit_size, Paging::NONE);

    // Deny user access to HEAP region
    Space_hst::access_ctrl (heap_base, heap_size, Paging::NONE);

    // Deny user access to TXT registers
    Space_hst::access_ctrl (txt_base, txt_size, Paging::NONE);

    // Grant user read/write access to TPM localities 0 + 1
    Space_hst::access_ctrl (0xfed40000, 2 * PAGE_SIZE (0), Paging::Permissions (Paging::U | Paging::W | Paging::R));

    // TXT registers and heap have already been mapped by launch()
    auto const heap_offs { MMAP_GLB_TXTH - (heap_base & ~Pte_hst::offs_mask (Pte_hst::bpl)) };
    auto ptr { heap_base + heap_offs };
    auto avl { size_t { min (Pte_hst::page_size (Pte_hst::bpl), heap_size) } };

    auto const efi_pre { consume<Data_efi_pre> (ptr, avl) };
    auto const pre_mle { consume<Data_pre_mle> (ptr, avl) };
    auto const pre_acm { consume<Data_pre_acm> (ptr, avl) };
    auto const acm_mle { consume<Data_acm_mle> (ptr, avl) };

    if (!efi_pre || !pre_mle || !pre_acm || !acm_mle) [[unlikely]]
        panic ("DRTM: Malformed TXT heap");

    trace (TRACE_DRTM, "DRTM: EFI-PRE v%u: %4lu", uint32_t { efi_pre->version }, uint64_t { efi_pre->size });
    trace (TRACE_DRTM, "DRTM: PRE-MLE v%u: %4lu", uint32_t { 0 },                uint64_t { pre_mle->size });
    trace (TRACE_DRTM, "DRTM: PRE-ACM v%u: %4lu", uint32_t { pre_acm->version }, uint64_t { pre_acm->size });
    trace (TRACE_DRTM, "DRTM: ACM-MLE v%u: %4lu", uint32_t { acm_mle->version }, uint64_t { acm_mle->size });

    // Consume extended heap elements
    if (pre_acm->version >= 6) [[likely]]
        parse_elem (reinterpret_cast<uintptr_t>(pre_acm + 1), pre_acm->size - sizeof (Data_pre_acm), heap_offs, std::to_underlying (Element::Type::PRE_ACM));
    if (acm_mle->version >= 8) [[likely]]
        parse_elem (reinterpret_cast<uintptr_t>(acm_mle + 1), acm_mle->size - sizeof (Data_acm_mle), heap_offs, std::to_underlying (Element::Type::ACM_MLE));

    // Override ACPI DMAR table with ACM-validated copy on the TXT heap (v5+)
    auto const dmar { acm_mle->version >= 5 ? view_buffer_as<uint8_t const> (reinterpret_cast<uintptr_t>(acm_mle), acm_mle->size, acm_mle->dmar_offset, max (uint32_t { sizeof (Acpi_table) }, uint32_t { acm_mle->dmar_size })) : nullptr };
    if (dmar) [[likely]]
        std::start_lifetime_as<Acpi_table const> (dmar)->validate (reinterpret_cast<uintptr_t>(dmar) - heap_offs, acm_mle->dmar_size);

    // Open locality 1 and enable secrets protection
    if (!command (Reg8::LOCALITY1_OPEN, Reg64::STS, STS::LOCALITY1) || !command (Reg8::SECRETS_SET, Reg64::E2STS, E2STS::SECRETS)) [[unlikely]]
        panic ("DRTM: TXT command failed");

    // Wake RLPs
    write (Space::PUBLIC, Reg32::MLE_JOIN, static_cast<uint32_t>(Kmem::ptr_to_phys (&Smx::mle_join)));

    if (pre_acm->caps & Acm::Cap::WAKEUP_GETSEC)
        Smx::wakeup();
    else if (pre_acm->caps & Acm::Cap::WAKEUP_MONITOR)
        *std::start_lifetime_as<Atomic<uint32_t>> (Ptab_hst::map (MMAP_GLB_MAP0, acm_mle->rlp_wakeup, Paging::W, Memattr::ram(), 1)) = 1;
}

void Txt::fini()
{
    // Abort if there was no measured launch
    if (!launched) [[unlikely]]
        return;

    // Close locality 1 and disable secrets protection
    if (command (Reg8::LOCALITY1_CLOSE, Reg64::STS, STS::LOCALITY1) || command (Reg8::SECRETS_CLR, Reg64::E2STS, E2STS::SECRETS)) [[unlikely]]
        trace (TRACE_ERROR, "%s: TXT command failed", __func__);

    // Exit measured launch environment
    Smx::sexit();
}
