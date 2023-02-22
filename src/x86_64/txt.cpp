/*
 * Trusted Execution Technology (TXT)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "acpi.hpp"
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

bool Txt::check_acm (uint32_t sinit_base, uint32_t sinit_size, uint32_t fms, uint32_t &acm_size, uint32_t &acm_caps, uint32_t &tpm_caps)
{
    auto const acm { static_cast<Acm::Header const *>(Hptp::map (MMAP_GLB_MAP0, sinit_base)) };

    // FIXME: Check that ACM total size fits into remap window

    // ACM must have correct type and fit into the range
    if (EXPECT_FALSE (acm->type != 2 || acm->subtype != 0 || sinit_size < 4 * acm->total_size))
        return false;

    auto const info { reinterpret_cast<Acm::Info const *>(reinterpret_cast<uintptr_t>(acm) + 4 * (acm->header_size + acm->scratch_size)) };

    // ACM UUID must be valid
    constexpr Uuid uuid { 0x18db46a77fc03aaa, 0x5a7f418d8f69ac2e };
    if (EXPECT_FALSE (info->uuid != uuid))
        return false;

    // FIXME: Check that "pch" and "cpu" list start/end are within remap window

    // ACM must support PRE-ACM version
    if (EXPECT_FALSE (info->max_ver_pre_acm < ver_pre_acm))
        return false;

    // ACM must support MLE header version
    if (EXPECT_FALSE (info->min_ver_mle_hdr > reinterpret_cast<Mle_header const *>(Kmem::sym_to_virt (&__head_mle))->version))
        return false;

    // ACM must support PCH
    auto const pch { acm->pch (info) };
    if (EXPECT_TRUE (pch) && EXPECT_FALSE (!reinterpret_cast<Acm::Id_pch const *>(pch->list())->match (pch->count, read (Space::PUBLIC, Reg64::DIDVID))))
        return false;

    // ACM must support CPU
    auto const cpu { acm->cpu (info) };
    if (EXPECT_TRUE (cpu) && EXPECT_FALSE (!reinterpret_cast<Acm::Id_cpu const *>(cpu->list())->match (cpu->count, fms, Msr::read (Msr::Reg64::IA32_PLATFORM_ID))))
        return false;

    // Patch out MLE caps as needed to work around ACM bugs (this will change the hash)
    for (unsigned i { 0 }; i < sizeof Acm::quirks / sizeof *Acm::quirks; i++)
        if (Acm::quirks[i].chipset == (acm->vendor << 16 | acm->chipset) && Acm::quirks[i].date == acm->date)
            reinterpret_cast<Mle_header *>(Kmem::sym_to_virt (&__head_mle))->mle_caps &= ~Acm::quirks[i].caps;

    acm_size = 4 * acm->total_size;
    acm_caps = info->capabilities;
    tpm_caps = acm->tpm (info);

    return true;
}

bool Txt::init_heap (uint32_t heap_base, uint32_t /*heap_size*/, uint32_t acm_caps, uint32_t tpm_caps, unsigned vcnt)
{
    // EFI to PRE
    auto const efi_pre { reinterpret_cast<Data_efi_pre const *>(MMAP_GLB_TXTH + (heap_base & Hpt::offs_mask (Hpt::bpl))) };

    // FIXME: Check that EFI_PRE fits entirely into heap_size

    // Number of CPUs must be within range
    if (EXPECT_FALSE (efi_pre->num_cpu < 1 || efi_pre->num_cpu > NUM_CPU))
        return false;

    // Platform type must be supported by SINIT
    auto const plat { efi_pre->plat() };
    if (EXPECT_FALSE ((plat == 1 && !(acm_caps & Acm::Cap::PLAT_CLIENT)) || (plat == 2 && !(acm_caps & Acm::Cap::PLAT_SERVER))))
        return false;

    // PRE to MLE
    auto const pre_mle { new (efi_pre->data.next()) Data_pre_mle { vcnt } };
    pre_mle->msr_ia32_mtrr_def_type = Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE);
    pre_mle->msr_ia32_misc_enable   = Msr::read (Msr::Reg64::IA32_MISC_ENABLE);
    pre_mle->msr_ia32_debugctl      = Msr::read (Msr::Reg64::IA32_DEBUGCTL);

    // FIXME: Check that EFI_PRE + PRE_MLE fits entirely into heap_size

    // Save MTRRs
    auto addr { reinterpret_cast<uintptr_t>(pre_mle + 1) };
    for (unsigned i { 0 }; i < vcnt; i++, addr += sizeof (Data_pre_mle::Mtrr))
        new (addr) Data_pre_mle::Mtrr { Mtrr::get_base (i), Mtrr::get_mask (i) };

    auto const header { reinterpret_cast<Mle_header const *>(Kmem::sym_to_virt (&__head_mle)) };

    // PRE to ACM
    auto const pre_acm { new (pre_mle->data.next()) Data_pre_acm };
    pre_acm->flags          = !!(tpm_caps & Acm::Tpm::MAX_PERFORMANCE);
    pre_acm->mle_ptab       = Kmem::sym_to_phys (&MLE_L2);
    pre_acm->mle_size       = header->mle_end - header->mle_start;
    pre_acm->mle_header     = reinterpret_cast<uintptr_t>(&HASH_HEAD);
    pre_acm->pmr_lo_base    = Kmem::sym_to_phys (&NOVA_HPAS);
    pre_acm->pmr_lo_size    = Multiboot::ea - pre_acm->pmr_lo_base;
    pre_acm->capabilities   = header->mle_caps & acm_caps;
    pre_acm->efi_rsdp       = Uefi::info.rsdp;

    // FIXME: Check that EFI_PRE + PRE_MLE + PRE_ACM fits entirely into heap_size

    // Add LOG element
    auto const tpm_log { Kmem::sym_to_phys (&MLE_TL) };
    pre_acm->data.size += acm_caps & Acm::Cap::TPM_20_TCG_LOG ?
                          (new (reinterpret_cast<uintptr_t>(pre_acm) + pre_acm->data.size) Element_log20_tcg { tpm_log, PAGE_SIZE (0) })->elem.size :
                          (new (reinterpret_cast<uintptr_t>(pre_acm) + pre_acm->data.size) Element_log20_txt { tpm_log, PAGE_SIZE (0) })->elem.size;

    // Add END element
    pre_acm->data.size += (new (reinterpret_cast<uintptr_t>(pre_acm) + pre_acm->data.size) Element_end)->elem.size;

    return true;
}

bool Txt::init_mtrr (uint64_t phys, uint64_t size, unsigned vcnt, unsigned bits)
{
    // Ensure size is page-aligned
    size = align_up (size, PAGE_SIZE (0));

    auto const mask { BIT64 (bits) - 1 };

    {   Cache_guard guard;

        // Disable all MTRRs and set default memory type as UC
        Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, CA_TYPE_MEM_UC);

        // Update variable MTRRs to map SINIT ACM as WB
        for (unsigned i { 0 }; i < vcnt; i++) {

            uint64_t b { 0 }, m { 0 };

            if (size) {
                auto const s { BIT64 (max_order (phys, size)) };
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
    if (EXPECT_FALSE (!(ecx & BIT (6))))
        return;

    // Enable SMX
    Cr::set_cr4 (Cr::get_cr4() | CR4_SMXE);

    // Abort if SMX capabilities are missing
    if (EXPECT_FALSE ((Smx::capabilities() & Smx::required) != Smx::required))
        return;

    // Map TXT registers
    Hptp::map (MMAP_GLB_TXTC, txt_base, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev(), 1);

    auto const heap_base { read (Space::PUBLIC, Reg32::HEAP_BASE) };
    auto const heap_size { read (Space::PUBLIC, Reg32::HEAP_SIZE) };

    // Abort if HEAP region is invalid
    if (EXPECT_FALSE (!heap_base || !heap_size))
        return;

    // Map TXT heap
    Hptp::map (MMAP_GLB_TXTH, heap_base, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram(), 2);

    // The rest of the function is pre-launch only
    if (launched)
        return;

    auto const sinit_base { read (Space::PUBLIC, Reg32::SINIT_BASE) };
    auto const sinit_size { read (Space::PUBLIC, Reg32::SINIT_SIZE) };

    // Abort if SINIT region is invalid
    if (EXPECT_FALSE (!sinit_base || !sinit_size))
        return;

    // Abort if the last launch failed
    auto const error { read (Space::PUBLIC, Reg32::ERRORCODE) };
    if (EXPECT_FALSE (error & BIT (31)))
        if (!(error & BIT (30)) || error >> 4 & BIT_RANGE (5, 0))
            return;

    // Abort if any MCE banks report errors
    auto const mce { static_cast<uint8_t>(Msr::read (Msr::Reg64::IA32_MCG_CAP)) };
    for (unsigned i { 0 }; i < mce; i++)
        if (EXPECT_FALSE (Msr::read (Msr::Arr64::IA32_MC_STATUS, 4, i) & BIT64 (63)))
            return;

    // Abort if MCE is in progress
    if (EXPECT_FALSE (Msr::read (Msr::Reg64::IA32_MCG_STATUS) & BIT (2)))
        return;

    // Abort if TPM initialization fails
    if (EXPECT_FALSE (!Tpm::init (false)))
        return;

    // Abort if SINIT ACM check fails
    if (EXPECT_FALSE (!check_acm (sinit_base, sinit_size, fms, acm_size, acm_caps, tpm_caps)))
        return;

    // Determine number of variable MTRRs
    auto const vcnt { Mtrr::get_vcnt() };

    // Abort if TXT heap initialization fails
    if (EXPECT_FALSE (!init_heap (heap_base, heap_size, acm_caps, tpm_caps, vcnt)))
        return;

    // Abort if MTRR initialization fails
    if (EXPECT_FALSE (!init_mtrr (sinit_base, acm_size, vcnt, acm_caps & Acm::Cap::MAXPHYADDR ? Memattr::kbits + Memattr::obits : 36)))
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

    if (EXPECT_TRUE (launched)) {

        // TXT registers and heap have already been mapped by launch()
        auto const efi_pre { reinterpret_cast<Data_efi_pre const *>(MMAP_GLB_TXTH + (read (Space::PUBLIC, Reg32::HEAP_BASE) & Hpt::offs_mask (Hpt::bpl))) };
        auto const pre_mle { reinterpret_cast<Data_pre_mle const *>(efi_pre->data.next()) };
        auto mtrr { pre_mle->mtrr() };

        {   Cache_guard guard;

            // Restore variable MTRRs and zap key bits
            for (unsigned i { 0 }; i < vcnt; i++, mtrr++) {
                Mtrr::set_base (i, mtrr->base);
                Mtrr::set_mask (i, mtrr->mask & mask);
            }

            // Restore MSRs
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE,  pre_mle->msr_ia32_mtrr_def_type);
            Msr::write (Msr::Reg64::IA32_MISC_ENABLE,    pre_mle->msr_ia32_misc_enable);
            Msr::write (Msr::Reg64::IA32_DEBUGCTL,       pre_mle->msr_ia32_debugctl);
        }

        // Enable SMX
        Cr::set_cr4 (Cr::get_cr4() | CR4_SMXE);

        // Reenable SMI
        Smx::smctrl();

    } else {

        // Check for buggy firmware that misprogrammed the MTRRs
        if (EXPECT_FALSE (!Mtrr::validate (vcnt, ~mask))) {

            Cache_guard guard;

            // Disable MTRRs
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) & ~BIT (11));

            // Fix up variable MTRRs and zap key bits
            for (unsigned i { 0 }; i < vcnt; i++)
                Mtrr::set_mask (i, Mtrr::get_mask (i) & mask);

            // Enable MTRRs
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) | BIT (11));
        }
    }
}

void Txt::parse_elem (Element const *e, uintptr_t l, uintptr_t o)
{
    if (EXPECT_FALSE (!e))
        return;

    // Iterate Extended Heap Elements
    for (; reinterpret_cast<uintptr_t>(e) < l; e = e->next()) {

        switch (e->type) {

            default:
                break;

            case Element::Type::END:
                return;

            // Override these ACPI tables with their ACM-validated copies on the TXT heap
            case Element::Type::MADT:
            case Element::Type::MCFG:
            case Element::Type::DPTR:
            case Element::Type::CEDT:
                reinterpret_cast<Acpi_table const *>(e->data())->validate (e->data() - o, true);
                break;

            case Element::Type::LOG20_TCG:
                auto const log { reinterpret_cast<Element_log20_tcg const *>(e) };
                Tpm_log::init (log->phys, log->size, log->off_next);
                break;
        }
    }
}

void Txt::init()
{
    // Abort if there was no measured launch
    if (EXPECT_FALSE (!launched))
        return;

    Tpm::init (true);

    auto const didvid { read (Space::PUBLIC, Reg64::DIDVID) };
    auto const verqpi { read (Space::PUBLIC, Reg32::VER_QPIIF) };

    trace (TRACE_DRTM, "DRTM: %04x:%04x (%#x) %s", static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16), static_cast<uint16_t>(didvid >> 32), verqpi & VER_QPIIF::PRD ? "PRD" : "DBG");

    auto const dpr { read (Space::PUBLIC, Reg32::DPR) };
    auto const dma_size { (dpr >> 4 & BIT_RANGE (7, 0)) * BIT (20) };
    auto const dma_base { (dpr & ~BIT_RANGE (11, 0)) - dma_size };

    // Deny DPR/TXT memory
    Space_hst::user_access (dma_base, dma_size, false);
    Space_hst::user_access (txt_base, txt_size, false);

    // Allow LOG and TPM localities 0/1
    Space_hst::user_access (Kmem::sym_to_phys (&MLE_TL), PAGE_SIZE (0), true);
    Space_hst::user_access (0xfed40000, 2 * PAGE_SIZE (0), true);

    auto const heap_base { read (Space::PUBLIC, Reg32::HEAP_BASE) };
    auto const heap_size { read (Space::PUBLIC, Reg32::HEAP_SIZE) };
    auto const heap_offs { MMAP_GLB_TXTH - (heap_base & ~Hpt::offs_mask (Hpt::bpl)) };

    trace (TRACE_DRTM, "DRTM: %#x/%#x SINIT %#x/%#x HEAP", read (Space::PUBLIC, Reg32::SINIT_BASE), read (Space::PUBLIC, Reg32::SINIT_SIZE), heap_base, heap_size);

    // TXT registers and heap have already been mapped by launch()
    auto const efi_pre { reinterpret_cast<Data_efi_pre const *>(heap_base + heap_offs) };
    auto const pre_mle { reinterpret_cast<Data_pre_mle const *>(efi_pre->data.next()) };
    auto const pre_acm { reinterpret_cast<Data_pre_acm const *>(pre_mle->data.next()) };
    auto const acm_mle { reinterpret_cast<Data_acm_mle const *>(pre_acm->data.next()) };

    trace (TRACE_DRTM, "DRTM: EFI-PRE v%u: %4lu", efi_pre->version, efi_pre->data.size);
    trace (TRACE_DRTM, "DRTM: PRE-MLE v%u: %4lu", pre_mle->version, pre_mle->data.size);
    trace (TRACE_DRTM, "DRTM: PRE-ACM v%u: %4lu", pre_acm->version, pre_acm->data.size);
    trace (TRACE_DRTM, "DRTM: ACM-MLE v%u: %4lu", acm_mle->version, acm_mle->data.size);

    // Consume extended heap elements
    parse_elem (pre_acm->elem(), pre_acm->data.next(), heap_offs);
    parse_elem (acm_mle->elem(), acm_mle->data.next(), heap_offs);

    // Override ACPI DMAR table with ACM-validated copy on the TXT heap (v5+)
    auto const dmar { reinterpret_cast<uintptr_t>(acm_mle) + acm_mle->dmar_offset };
    reinterpret_cast<Acpi_table const *>(dmar)->validate (dmar - heap_offs, true);

    // Clear "success" from errorcode (because a soft reset won't)
    write (Space::PRIVATE, Reg32::ERRORCODE, 0);

    // Open locality 1 and enable secrets protection
    if (!command (Reg8::LOCALITY1_OPEN, Reg64::STS, STS::LOCALITY1) || !command (Reg8::SECRETS_SET, Reg64::E2STS, E2STS::SECRETS))
        trace (TRACE_ERROR, "%s: TXT command failed", __func__);

    // Wake RLPs
    write (Space::PUBLIC, Reg32::MLE_JOIN, static_cast<uint32_t>(Kmem::ptr_to_phys (&Smx::mle_join)));

    if (pre_acm->capabilities & Acm::Cap::WAKEUP_GETSEC)
        Smx::wakeup();
    else if (pre_acm->capabilities & Acm::Cap::WAKEUP_MONITOR)
        *static_cast<Atomic<uint32_t> *>(Hptp::map (MMAP_GLB_MAP0, acm_mle->rlp_wakeup, Paging::W, Memattr::ram(), 1)) = 1;
}

void Txt::fini()
{
    // Abort if there was no measured launch
    if (EXPECT_FALSE (!launched))
        return;

    // Close locality 1 and disable secrets protection
    if (command (Reg8::LOCALITY1_CLOSE, Reg64::STS, STS::LOCALITY1) || command (Reg8::SECRETS_CLR, Reg64::E2STS, E2STS::SECRETS))
        trace (TRACE_ERROR, "%s: TXT command failed", __func__);

    // Exit measured launch environment
    Smx::sexit();
}
