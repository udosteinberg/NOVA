/*
 * Trusted Platform Module (TPM)
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

#include "stdio.hpp"
#include "tpm.hpp"
#include "util.hpp"

bool Tpm::init (bool full)
{
    uint32_t didvid;

    // Determine TPM interface
    switch (Interface::type()) {

        case 0xf:       // FIFO TIS
        case 0x0:       // FIFO PTP
            Fifo::init (didvid);
            break;

        case 0x1:       // CRB
            Crb::init (didvid);
            break;

        default:        // Unknown
            return false;
    }

    if (!request (Locality::L0))
        return false;

    if (full) {

        // Determine TPM properties
        switch (fam) {

            case Family::TPM_12:
                v1_cap_tpm_properties (num_pcr, Tpm12_ptg::Type::PTG_PCR_COUNT);
                v1_cap_tpm_properties (tpm_mfr, Tpm12_ptg::Type::PTG_MANUFACTURER);
                v1_cap_tpm_properties (max_buf, Tpm12_ptg::Type::PTG_INPUT_BUFFER);
                hash.add (Tpm_alg::Type::SHA1_160);
                break;

            case Family::TPM_20:
                v2_cap_tpm_properties();
                v2_cap_pcrs();
                break;
        }

        auto const vendor { __builtin_bswap32 (tpm_mfr) };

        trace (TRACE_TPM, "TPM%u: %04x:%04x %s %s ALG:%u PCR:%u BUF:%u %4.4s", std::to_underlying (fam) + 1,
               static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16),
               ift == Iftype::FIFO ? "FIFO" : ift == Iftype::CRB ? "CRB" : "?",
               fam == Family::TPM_20 ? "2.0" : fam == Family::TPM_12 ? "1.2" : "?",
               hash.count(), num_pcr, max_buf, reinterpret_cast<char const *>(&vendor));
    }

    return release (Locality::L0);
}

bool Tpm::Fifo::send (Locality l, Cmd const &cmd)
{
    auto p { reinterpret_cast<uint8_t const *>(&cmd) };
    auto s { cmd.size() };

    // Transmit command
    for (unsigned i { 0 }, b { 0 }; i < s; i++, b--) {

        // Update burstcount
        if (EXPECT_FALSE (!b && !burstcount (l, b)))
            return false;

        write (l, Reg8::FIFO_DATA, *p++);
    }

    // Check for complete transmission
    return wait_done (l, BIT (3));
}

bool Tpm::Fifo::recv (Locality l)
{
    auto p { buffer };
    auto s { sizeof (Res) };

    // Transmit response
    for (unsigned i { 0 }, b { 0 }; i < s; i++, b--) {

        // Update burstcount
        if (EXPECT_FALSE (!b && !burstcount (l, b)))
            return false;

        // Update response size
        if (EXPECT_FALSE (p - buffer == sizeof (Hdr)))
            s = min (sizeof (buffer), static_cast<size_t>(reinterpret_cast<Res *>(buffer)->size()));

        *p++ = read (l, Reg8::FIFO_DATA);
    }

    // Check for complete transmission
    return wait_done (l, BIT (4));
}

bool Tpm::Fifo::execute (Locality l, Cmd const &cmd)
{
    // Enter ready state
    if (EXPECT_FALSE (!state (l, false)))
        return false;

    bool const ret { send (l, cmd) && exec (l) && recv (l) };

    // Enter idle state
    return state (l, true) && ret;
}

bool Tpm::Crb::send (Locality l, Cmd const &cmd)
{
    auto p { reinterpret_cast<uint8_t const *>(&cmd) };
    auto s { cmd.size() };

    // Transmit command
    for (unsigned i { 0 }; i < s; i++)
        write (l, Reg8 { std::to_underlying (Reg8::CRB_DATA) + i }, *p++);

    return true;
}

bool Tpm::Crb::recv (Locality l)
{
    auto p { buffer };
    auto s { sizeof (Res) };

    // Transmit response
    for (unsigned i { 0 }; i < s; i++) {

        // Update response size
        if (EXPECT_FALSE (p - buffer == sizeof (Hdr)))
            s = min (sizeof (buffer), static_cast<size_t>(reinterpret_cast<Res *>(buffer)->size()));

        *p++ = read (l, Reg8 { std::to_underlying (Reg8::CRB_DATA) + i });
    }

    return true;
}

bool Tpm::Crb::execute (Locality l, Cmd const &cmd)
{
    // Enter ready state
    if (EXPECT_FALSE (!state (l, BIT (0))))
        return false;

    bool const ret { send (l, cmd) && exec (l) && recv (l) };

    // Enter idle state
    return state (l, BIT (1)) && ret;
}

bool Tpm::execute (Locality l, Cmd const &cmd)
{
    if (EXPECT_FALSE (!(ift == Iftype::CRB ? Crb::execute (l, cmd) : Fifo::execute (l, cmd))))
        return false;

    auto const r { reinterpret_cast<Res const *>(buffer) };

    // Success
    if (EXPECT_TRUE (r->type() == Tpm_rc::Type::RC_SUCCESS))
        return true;

    // Error
    auto const e { std::to_underlying (r->type()) };

    if (e & BIT (7))    // Format-One
        trace (TRACE_ERROR, "TPM2: CMD %#x failed with %#x (%c:%u E:%#x)", std::to_underlying (cmd.type()), e, e & BIT (6) ? 'P' : e & BIT (11) ? 'S' : 'H', e >> 8 & BIT_RANGE (2, 0), e & BIT_RANGE (5, 0));
    else                // Format-Zero
        trace (TRACE_ERROR, "TPM2: CMD %#x failed with %#x", std::to_underlying (cmd.type()), e);

    return false;
}

bool Tpm::v2_cap_pcrs()
{
    // Number of capabilities that fit in the buffer (assumes 3 bytes for 24 PCRs)
    constexpr auto num { (sizeof (buffer) - sizeof (Tpm20_get_capability_res) - sizeof (Tpml_pcr_selection)) / (sizeof (Tpms_pcr_selection) + 3) };

    auto const r { reinterpret_cast<Tpm20_get_capability_res const *>(buffer) };

    // Abort if the command failed or returned the wrong type
    if (EXPECT_FALSE (!execute (Locality::L0, Tpm20_get_capability { Tpm20_cap::Type::CAP_PCRS, Tpm20_ptg::Type { 0 }, num }) || r->data.type() != Tpm20_cap::Type::CAP_PCRS))
        return false;

    // List type is Tpml_pcr_selection
    auto const tpml_pcr { r->data.next<Tpml_pcr_selection>() };

    // Item type is Tpms_pcr_selection
    auto p { tpml_pcr->next() };

    // Enumerate supported hash algorithms that have PCRs allocated to them
    for (auto i { tpml_pcr->size() }; i--; p = p->next())
        if (p->sel.pcrs())
            hash.add (p->alg.type());

    return true;
}

bool Tpm::v2_cap_tpm_properties()
{
    // Number of capabilities that fit in the buffer
    constexpr auto num { (sizeof (buffer) - sizeof (Tpm20_get_capability_res) - sizeof (Tpml_tagged_tpm_property)) / sizeof (Tpms_tagged_property) };

    auto const r { reinterpret_cast<Tpm20_get_capability_res const *>(buffer) };

    // Iterate over all tags in the fixed group
    for (unsigned tag { 0x100 }; tag < 0x200; tag++) {

        // Abort if the command failed or returned the wrong type
        if (EXPECT_FALSE (!execute (Locality::L0, Tpm20_get_capability { Tpm20_cap::Type::CAP_TPM_PROPERTIES, Tpm20_ptg::Type { tag }, num }) || r->data.type() != Tpm20_cap::Type::CAP_TPM_PROPERTIES))
            return false;

        // List type is Tpml_tagged_tpm_property
        auto const tpml_ptg { r->data.next<Tpml_tagged_tpm_property>() };

        // Item type is Tpms_tagged_property
        auto p { tpml_ptg->next() };

        for (auto i { tpml_ptg->size() }; i--; p = p->next()) {

            switch (p->ptg.type()) {
                case Tpm20_ptg::Type::PTG_MANUFACTURER: tpm_mfr = p->val; break;
                case Tpm20_ptg::Type::PTG_INPUT_BUFFER: max_buf = p->val; break;
                case Tpm20_ptg::Type::PTG_PCR_COUNT:    num_pcr = p->val; break;
                case Tpm20_ptg::Type::PTG_MAX_DIGEST:   max_dig = p->val; break;
            }

            tag = std::to_underlying (p->ptg.type());
        }

        if (EXPECT_TRUE (!r->more))
            break;
    }

    return true;
}

bool Tpm::v1_cap_tpm_properties (uint32_t &val, Tpm12_ptg::Type t)
{
    auto const r { reinterpret_cast<Tpm12_get_capability_res const *>(buffer) };

    // Abort if the command failed or returned the wrong size
    if (EXPECT_FALSE (!execute (Locality::L0, Tpm12_get_capability { t }) || r->len != sizeof (uint32_t)))
        return false;

    val = *reinterpret_cast<be_uint32_t const *>(r + 1);

    return true;
}
