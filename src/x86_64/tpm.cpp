/*
 * Trusted Platform Module (TPM)
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

#include "stdio.hpp"
#include "tpm.hpp"
#include "util.hpp"

bool Tpm::init (Locality l)
{
    if (static_cast<Interface>(read (l, Reg32::INTERFACE_ID) & BIT_RANGE (3, 0)) == Interface::CRB)
        return false;

    if (EXPECT_FALSE (!request (l)))
        return false;

    if ((read (l, Reg32::INTF_CAPABILITY) >> 28 & BIT_RANGE (2, 0)) == 3) {
        fam = static_cast<Family>(read (l, Reg32::STATUS) >> 26 & BIT_RANGE (1, 0));
        ifc = Interface::FIFO_PTP;
    } else {
        fam = Family::TPM_12;           // STATUS[31:24] are undefined
        ifc = Interface::FIFO_TIS;
    }

    if (l == Locality::L0)
        return make_ready (l) && release (l);

    // Enumerate properties
    switch (fam) {

        case Family::TPM_12:
            v1_cap_tpm_properties (num_pcr, Tpm12_ptg::Type::PTG_PCR_COUNT);
            v1_cap_tpm_properties (tpm_mfr, Tpm12_ptg::Type::PTG_MANUFACTURER);
            v1_cap_tpm_properties (max_buf, Tpm12_ptg::Type::PTG_INPUT_BUFFER);
            break;

        case Family::TPM_20:
            v2_cap_tpm_properties();
            v2_cap_pcrs();
            break;
    }

    auto const didvid { read (l, Reg32::DIDVID) };
    auto const vendor { __builtin_bswap32 (tpm_mfr) };

    trace (TRACE_TPM, "TPM%u: %04x:%04x (%#x) %s %s PCR:%u BUF:%u %4.4s", std::to_underlying (fam) + 1,
           static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16), read (l, Reg8::RID),
           fam == Family::TPM_20 ? "2.0" : fam == Family::TPM_12 ? "1.2" : "?",
           ifc == Interface::FIFO_PTP ? "PTP" : ifc == Interface::FIFO_TIS ? "TIS" : "?",
           num_pcr, max_buf, reinterpret_cast<char const *>(&vendor));

    return release (l);
}

bool Tpm::send (Locality l, Cmd const &cmd)
{
    // Wait for command readiness (within Timeout::B)
    if (!poll_status (l, Status::COMMAND_READY, Status::COMMAND_READY))
        return false;

    // After Status::COMMAND_READY = 1, BurstCount must be > 0 within Timeout::A
    unsigned b { 0 }, s { cmd.size() };

    // Transmit command
    for (auto p { reinterpret_cast<uint8_t const *>(&cmd) }; s--; b--, write (l, Reg8::DATA_FIFO, *p++))
        if (EXPECT_FALSE (!b && !burstcount (l, b)))
            return false;

    // Check for complete transmission
    return done_send (l);
}

bool Tpm::recv (Locality l)
{
    // Wait for command completion (within Timeout::B)
    if (!poll_status (l, Status::STATUS_VALID | Status::DATA_AVAIL, Status::STATUS_VALID | Status::DATA_AVAIL))
        return false;

    // After Status::DATA_AVAIL = 1, BurstCount must be > 0 within Timeout::A
    unsigned b { 0 }, s { sizeof (Res) }, m { sizeof (buffer) };

    // Transmit response
    for (auto p { buffer }; s--; b--, *p++ = read (l, Reg8::DATA_FIFO)) {

        if (EXPECT_FALSE (!b && !burstcount (l, b)))
            return false;

        if (EXPECT_FALSE (p - buffer == sizeof (Hdr)))
            s = min (m, reinterpret_cast<Res *>(buffer)->size()) - 7;
    }

    // Check for complete transmission
    return done_recv (l);
}

bool Tpm::exec (Locality l, Cmd const &cmd)
{
    {   Cmd_guard guard { l };

        // Send Phase
        if (EXPECT_FALSE (!send (l, cmd))) {
            trace (TRACE_ERROR, "TPM2: CMD %#x send error", std::to_underlying (cmd.type()));
            return false;
        }

        // Execute Command
        write (l, Reg32::STATUS, Status::TPM_GO);

        // Recv Phase
        if (EXPECT_FALSE (!recv (l))) {
            trace (TRACE_ERROR, "TPM2: CMD %#x recv error", std::to_underlying (cmd.type()));
            return false;
        }
    }

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

bool Tpm::v2_pcr_read (unsigned pcr)
{
    // Abort if the command failed
    if (EXPECT_FALSE (!exec (nova, Tpm20_pcr_read { pcr })))
        return false;

    // List type is Tpml_pcr_selection
    auto const tpml_pcr { reinterpret_cast<Tpml_pcr_selection const *>(reinterpret_cast<Tpm20_pcr_read_res const *>(buffer) + 1) };

    // Item type is Tpms_pcr_selection
    auto p { tpml_pcr->next() };

    // Iterate over all list items
    for (auto i { tpml_pcr->size() }; i--; p = p->next()) ;

    // List type is Tpml_digest
    auto const tpml_dig { reinterpret_cast<Tpml_digest const *>(p) };

    // Item type is Tpm2b_digest
    auto d { tpml_dig->next() };

    // Iterate over all list items
    for (auto i { tpml_dig->size() }; i--; d = d->next()) {

        char digest[2 * d->size() + 1];     // Digest buffer (2 chars per byte + terminator)
        digest_render (digest, d->dgst(), d->size());

        trace (TRACE_TPM, "TPM2: # %2u: %s", pcr, digest);
    }

    return true;
}

bool Tpm::v1_pcr_read (unsigned pcr)
{
    // Abort if the command failed
    if (EXPECT_FALSE (!exec (nova, Tpm12_pcr_read { pcr })))
        return false;

    char digest[2 * 20 + 1];                // Digest buffer (2 chars per byte + terminator)
    digest_render (digest, reinterpret_cast<Tpm12_pcr_read_res const *>(buffer)->sha1_160, 20);

    trace (TRACE_TPM, "TPM1: # %2u: %s", pcr, digest);

    return true;
}

bool Tpm::v2_cap_pcrs()
{
    // Number of capabilities that fit in the buffer (assumes 3 bytes for 24 PCRs)
    constexpr auto num { (sizeof (buffer) - sizeof (Tpm20_get_capability_res) - sizeof (Tpml_pcr_selection)) / (sizeof (Tpms_pcr_selection) + 3) };

    auto const r { reinterpret_cast<Tpm20_get_capability_res const *>(buffer) };

    // Abort if the command failed or returned the wrong type
    if (EXPECT_FALSE (!exec (nova, Tpm20_get_capability { Tpm20_cap::Type::CAP_PCRS, Tpm20_ptg::Type { 0 }, num }) || r->data.type() != Tpm20_cap::Type::CAP_PCRS))
        return false;

    // List type is Tpml_pcr_selection
    auto const tpml_pcr { r->data.next<Tpml_pcr_selection>() };

    // Item type is Tpms_pcr_selection
    auto p { tpml_pcr->next() };

    // Enumerate supported hash algorithms that have PCRs allocated to them
    for (auto i { tpml_pcr->size() }; i--; p = p->next())
        if (p->sel.pcrs())
            switch (p->alg.type()) {
                case Tpm_alg::Type::ALG_SHA1_160:   hash_algorithms |= Hash_alg::SHA1_160;  break;
                case Tpm_alg::Type::ALG_SHA2_256:   hash_algorithms |= Hash_alg::SHA2_256;  break;
                case Tpm_alg::Type::ALG_SHA2_384:   hash_algorithms |= Hash_alg::SHA2_384;  break;
                case Tpm_alg::Type::ALG_SHA2_512:   hash_algorithms |= Hash_alg::SHA2_512;  break;
                case Tpm_alg::Type::ALG_SM3_256:    hash_algorithms |= Hash_alg::SM3_256;   break;
                case Tpm_alg::Type::ALG_SHA3_256:   hash_algorithms |= Hash_alg::SHA3_256;  break;
                case Tpm_alg::Type::ALG_SHA3_384:   hash_algorithms |= Hash_alg::SHA3_384;  break;
                case Tpm_alg::Type::ALG_SHA3_512:   hash_algorithms |= Hash_alg::SHA3_512;  break;
            }

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
        if (EXPECT_FALSE (!exec (nova, Tpm20_get_capability { Tpm20_cap::Type::CAP_TPM_PROPERTIES, Tpm20_ptg::Type { tag }, num }) || r->data.type() != Tpm20_cap::Type::CAP_TPM_PROPERTIES))
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
    if (EXPECT_FALSE (!exec (nova, Tpm12_get_capability { t }) || r->len != sizeof (uint32_t)))
        return false;

    val = *reinterpret_cast<be_uint32_t const *>(r + 1);

    return true;
}
