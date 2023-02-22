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

bool Txt::check_acm (Mle_header *hdr, uint32_t sinit_base, uint32_t sinit_size, uint32_t fms, uint32_t &acm_size, uint32_t &acm_caps, uint32_t &tpm_caps)
{
    auto const acm { static_cast<Acm::Header const *>(Hptp::map (MMAP_GLB_MAP0, sinit_base)) };

    // FIXME: Check that ACM total size fits into remap window

    // ACM must have correct type and fit into the range
    if (acm->type != 2 || acm->subtype != 0 || sinit_size < 4 * acm->total_size) [[unlikely]]
        return false;

    auto const info { reinterpret_cast<Acm::Info const *>(reinterpret_cast<uintptr_t>(acm) + 4 * (acm->header_size + acm->scratch_size)) };

    // ACM UUID must be valid
    if (info->uuid != Uuid { 0x18db46a77fc03aaa, 0x5a7f418d8f69ac2e }) [[unlikely]]
        return false;

    // FIXME: Check that "pch" and "cpu" list start/end are within remap window

    // ACM must support PRE-ACM version
    if (info->max_ver_pre_acm < ver_pre_acm) [[unlikely]]
        return false;

    // ACM must support MLE header version
    if (info->min_ver_mle_hdr > hdr->version) [[unlikely]]
        return false;

    // ACM must support PCH
    auto const pch { acm->pch (info) };
    if (pch && !reinterpret_cast<Acm::Id_pch const *>(pch->list())->match (pch->count, read (Space::PUBLIC, Reg64::DIDVID))) [[unlikely]]
        return false;

    // ACM must support CPU
    auto const cpu { acm->cpu (info) };
    if (cpu && !reinterpret_cast<Acm::Id_cpu const *>(cpu->list())->match (cpu->count, fms, Msr::read (Msr::Reg64::IA32_PLATFORM_ID))) [[unlikely]]
        return false;

    // Patch out MLE caps as needed to work around ACM bugs (this will change the hash)
    for (unsigned i { 0 }; i < sizeof Acm::quirks / sizeof *Acm::quirks; i++)
        if (Acm::quirks[i].chipset == (acm->vendor << 16 | acm->chipset) && Acm::quirks[i].date == acm->date) [[unlikely]]
            hdr->mle_caps = hdr->mle_caps & ~Acm::quirks[i].caps;

    acm_size = 4 * acm->total_size;
    acm_caps = info->capabilities;
    tpm_caps = acm->tpm (info);

    return true;
}

bool Txt::init_heap (Mle_header *hdr, uint32_t heap_base, uint32_t /*heap_size*/, uint32_t acm_caps, uint32_t tpm_caps, unsigned vcnt)
{
    // EFI to PRE
    auto const efi_pre { reinterpret_cast<Data_efi_pre const *>(MMAP_GLB_TXTH + (heap_base & Hpt::offs_mask (Hpt::bpl))) };

    // FIXME: Check that EFI_PRE fits entirely into heap_size

    // Number of CPUs must be within range
    if (efi_pre->num_cpu < 1 || efi_pre->num_cpu > NUM_CPU) [[unlikely]]
        return false;

    // Platform type must be supported by SINIT
    auto const plat { efi_pre->plat() };
    if ((plat == 1 && !(acm_caps & Acm::Cap::PLAT_CLIENT)) || (plat == 2 && !(acm_caps & Acm::Cap::PLAT_SERVER))) [[unlikely]]
        return false;

    // PRE to MLE
    auto const pre_mle { new (efi_pre->data.next()) Data_pre_mle
    {
        .data               { Data { sizeof (Data_pre_mle) + vcnt * sizeof (Data_pre_mle::Mtrr_backup) } },
        .ia32_mtrr_def_type { Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) },
        .ia32_misc_enable   { Msr::read (Msr::Reg64::IA32_MISC_ENABLE) },
        .ia32_debugctl      { Msr::read (Msr::Reg64::IA32_DEBUGCTL) },
    } };

    // FIXME: Check that EFI_PRE + PRE_MLE fits entirely into heap_size

    // Save MTRRs
    auto mtrr { pre_mle->mtrr() };
    for (unsigned i { 0 }; i < vcnt; i++, mtrr++)
        new (mtrr) Data_pre_mle::Mtrr_backup { Mtrr::get_base (i), Mtrr::get_mask (i) };

    // PRE to ACM
    auto const pre_acm { new (pre_mle->data.next()) Data_pre_acm
    {
        .flags              { !!(tpm_caps & Acm::Tpm::MAX_PERFORMANCE) },
        .mle_ptab           { Kmem::sym_to_phys (&MLE_L2) },
        .mle_size           { hdr->mle_end - hdr->mle_start },
        .mle_header         { reinterpret_cast<uintptr_t>(&HASH_HEAD) },
        .pmr_lo_base        { Kmem::sym_to_phys (&NOVA_HPAS) },
        .pmr_lo_size        { Multiboot::ea - Kmem::sym_to_phys (&NOVA_HPAS) },
        .caps               { hdr->mle_caps & acm_caps },
        .rsdp               { Uefi::info.rsdp },
    } };

    // FIXME: Check that EFI_PRE + PRE_MLE + PRE_ACM fits entirely into heap_size

    // Add LOG element
    auto const tpm_log { Kmem::sym_to_phys (&MLE_TL) };
    pre_acm->append (acm_caps & Acm::Cap::TPM_20_TCG_LOG ?
                          (new (pre_acm->data.next()) Element_log20_tcg { tpm_log, PAGE_SIZE (0) })->elem :
                          (new (pre_acm->data.next()) Element_log20_txt { tpm_log, PAGE_SIZE (0) })->elem);

    // Add END element
    pre_acm->append ((new (pre_acm->data.next()) Element_end)->elem);

    return true;
}

bool Txt::init_mtrr (uint64_t phys, uint64_t size, unsigned vcnt, unsigned bits)
{
    // Ensure size is a multiple of PAGE_SIZE
    size = aligned_up (PAGE_SIZE (0), size);

    auto const mask { BIT64 (bits) - 1 };

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
    Hptp::map (MMAP_GLB_TXTC, txt_base, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev(), 1);

    auto const heap_base { read (Space::PUBLIC, Reg32::HEAP_BASE) };
    auto const heap_size { read (Space::PUBLIC, Reg32::HEAP_SIZE) };

    // Abort if HEAP region is invalid
    if (!heap_base || !heap_size) [[unlikely]]
        return;

    // Map TXT heap
    Hptp::map (MMAP_GLB_TXTH, heap_base, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram(), 2);

    // The rest of the function is pre-launch only
    if (launched)
        return;

    auto const sinit_base { read (Space::PUBLIC, Reg32::SINIT_BASE) };
    auto const sinit_size { read (Space::PUBLIC, Reg32::SINIT_SIZE) };

    // Abort if SINIT region is invalid
    if (!sinit_base || !sinit_size) [[unlikely]]
        return;

    // Abort if the last launch failed
    auto const error { read (Space::PUBLIC, Reg32::ERRORCODE) };
    if (error & BIT (31)) [[unlikely]]
        if (!(error & BIT (30)) || error >> 4 & BIT_RANGE (5, 0))
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

    auto const mle { reinterpret_cast<Mle_header *>(Kmem::sym_to_virt (&__head_mle)) };

    // Abort if SINIT ACM check fails
    if (!check_acm (mle, sinit_base, sinit_size, fms, acm_size, acm_caps, tpm_caps)) [[unlikely]]
        return;

    // Determine number of variable MTRRs
    auto const vcnt { Mtrr::get_vcnt() };

    // Abort if TXT heap initialization fails
    if (!init_heap (mle, heap_base, heap_size, acm_caps, tpm_caps, vcnt)) [[unlikely]]
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
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) & ~BIT (11));

            // Fix up variable MTRRs and zap key bits
            for (unsigned i { 0 }; i < vcnt; i++)
                Mtrr::set_mask (i, Mtrr::get_mask (i) & mask);

            // Enable MTRRs
            Msr::write (Msr::Reg64::IA32_MTRR_DEF_TYPE, Msr::read (Msr::Reg64::IA32_MTRR_DEF_TYPE) | BIT (11));
        }
    }
}

void Txt::parse_elem (Element const *e, void const *l, uintptr_t o)
{
    if (!e) [[unlikely]]
        return;

    // Iterate Extended Heap Elements
    for (; static_cast<void const *>(e) < l; e = e->get_next()) {

        switch (e->get_type()) {

            default:
                break;

            case Element::Type::END:
                return;

            // Override these ACPI tables with their ACM-validated copies on the TXT heap
            case Element::Type::MADT:
            case Element::Type::MCFG:
            case Element::Type::DPTR:
            case Element::Type::CEDT:
                reinterpret_cast<Acpi_table const *>(e->get_data())->validate (e->get_data() - o);
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
    if (!launched) [[unlikely]]
        return;

    Tpm::init (true);

    auto const dv1 { read (Space::PUBLIC, Reg64::DIDVID) };
    auto const dv2 { read (Space::PUBLIC, Reg64::DIDVID2) };
    auto const ver { read (Space::PUBLIC, Reg32::VER_QPIIF) };

    trace (TRACE_DRTM, "DRTM: %04x:%04x (%#x) %04x:%04x (%s)",
           static_cast<uint16_t>(dv1), static_cast<uint16_t>(dv1 >> 16), static_cast<uint16_t>(dv1 >> 32),
           static_cast<uint16_t>(dv2), static_cast<uint16_t>(dv2 >> 16), ver & VER_QPIIF::PRD ? "PRD" : "DBG");

    auto const dpr { read (Space::PUBLIC, Reg32::DPR) };
    auto const dma_size { (dpr >> 4 & BIT_RANGE (7, 0)) * BIT (20) };
    auto const dma_base { (dpr & ~BIT_RANGE (11, 0)) - dma_size };

    // Reserve DPR/TXT region
    Space_hst::access_ctrl (dma_base, dma_size, Paging::NONE);
    Space_hst::access_ctrl (txt_base, txt_size, Paging::NONE);

    // Grant user read/write access to TPM localities 0/1 and read access to event log
    Space_hst::access_ctrl (0xfed40000, 2 * PAGE_SIZE (0), Paging::Permissions (Paging::U | Paging::W | Paging::R));
    Space_hst::access_ctrl (Kmem::sym_to_phys (&MLE_TL), PAGE_SIZE (0), Paging::Permissions (Paging::U | Paging::R));

    auto const heap_base { read (Space::PUBLIC, Reg32::HEAP_BASE) };
    auto const heap_size { read (Space::PUBLIC, Reg32::HEAP_SIZE) };
    auto const heap_offs { MMAP_GLB_TXTH - (heap_base & ~Hpt::offs_mask (Hpt::bpl)) };

    trace (TRACE_DRTM, "DRTM: %#x/%#x SINIT %#x/%#x HEAP", read (Space::PUBLIC, Reg32::SINIT_BASE), read (Space::PUBLIC, Reg32::SINIT_SIZE), heap_base, heap_size);

    // TXT registers and heap have already been mapped by launch()
    auto const efi_pre { reinterpret_cast<Data_efi_pre const *>(heap_base + heap_offs) };
    auto const pre_mle { reinterpret_cast<Data_pre_mle const *>(efi_pre->data.next()) };
    auto const pre_acm { reinterpret_cast<Data_pre_acm const *>(pre_mle->data.next()) };
    auto const acm_mle { reinterpret_cast<Data_acm_mle const *>(pre_acm->data.next()) };

    trace (TRACE_DRTM, "DRTM: EFI-PRE v%u: %4lu", uint32_t { efi_pre->version }, uint64_t { efi_pre->data.size });
    trace (TRACE_DRTM, "DRTM: PRE-MLE v%u: %4lu", uint32_t { 0 },                uint64_t { pre_mle->data.size });
    trace (TRACE_DRTM, "DRTM: PRE-ACM v%u: %4lu", uint32_t { pre_acm->version }, uint64_t { pre_acm->data.size });
    trace (TRACE_DRTM, "DRTM: ACM-MLE v%u: %4lu", uint32_t { acm_mle->version }, uint64_t { acm_mle->data.size });

    // Consume extended heap elements
    parse_elem (pre_acm->elem(), pre_acm->data.next(), heap_offs);
    parse_elem (acm_mle->elem(), acm_mle->data.next(), heap_offs);

    // Override ACPI DMAR table with ACM-validated copy on the TXT heap (v5+)
    auto const dmar { reinterpret_cast<uintptr_t>(acm_mle) + acm_mle->dmar_offset };
    reinterpret_cast<Acpi_table const *>(dmar)->validate (dmar - heap_offs);

    // Clear "success" from errorcode (because a soft reset won't)
    write (Space::PRIVATE, Reg32::ERRORCODE, 0);

    // Open locality 1 and enable secrets protection
    if (!command (Reg8::LOCALITY1_OPEN, Reg64::STS, STS::LOCALITY1) || !command (Reg8::SECRETS_SET, Reg64::E2STS, E2STS::SECRETS)) [[unlikely]]
        trace (TRACE_ERROR, "%s: TXT command failed", __func__);

    // Wake RLPs
    write (Space::PUBLIC, Reg32::MLE_JOIN, static_cast<uint32_t>(Kmem::ptr_to_phys (&Smx::mle_join)));

    if (pre_acm->caps & Acm::Cap::WAKEUP_GETSEC)
        Smx::wakeup();
    else if (pre_acm->caps & Acm::Cap::WAKEUP_MONITOR)
        *static_cast<Atomic<uint32_t> *>(Hptp::map (MMAP_GLB_MAP0, acm_mle->rlp_wakeup, Paging::W, Memattr::ram(), 1)) = 1;
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
