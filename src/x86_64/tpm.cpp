/*
 * Trusted Platform Module (TPM)
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

#include "stdio.hpp"
#include "tpm.hpp"
#include "util.hpp"

bool Tpm::init (bool full)
{
    uint32_t did_vid;

    // Determine TPM interface type
    switch (Fifo::iftype()) {

        case Iftype::LEGACY:
            [[fallthrough]];

        case Iftype::FIFO:
            // Determine TPM family and DID/VID
            if (!Fifo::init (did_vid)) [[unlikely]]
                return false;
            f_request = &Fifo::request;
            f_release = &Fifo::release;
            f_execute = &Fifo::execute;
            break;

        case Iftype::CRB:
            // Determine TPM family and DID/VID
            if (!Crb::init (did_vid)) [[unlikely]]
                return false;
            f_request = &Crb::request;
            f_release = &Crb::release;
            f_execute = &Crb::execute;
            break;

        default:
            return false;
    }

    constexpr auto l { Locality::L0 };

    // Request locality
    if (!request (l)) [[unlikely]]
        return false;

    // Determine TPM capabilities if full init
    bool const ret { !full || (cap_tpm_properties (l) && cap_pcrs (l)) };

    if (full)
        trace (TRACE_TPM, "TPM2: %04x:%04x %.4s %.16s v%u.%u %s REV:%u ALG:%u PCR:%u MAX:%u/%u",
               static_cast<uint16_t>(did_vid), static_cast<uint16_t>(did_vid >> 16), tpm_mfr, tpm_ven,
               static_cast<uint16_t>(ver_fw1 >> 16), static_cast<uint16_t>(ver_fw1),
               f_execute == &Fifo::execute ? "FIFO" : f_execute == &Crb::execute ? "CRB" : "?",
               tpm_rev, hash.count(), num_pcr, max_cmd, max_res);

    // Release locality
    return release (l) && ret;
}

bool Tpm::invoke (Locality l, Cmd &&cmd, Hmac_session *session)
{
    // Determine number of handles in command and response
    auto const c { cmd.h_cmd() };
    auto const r { cmd.h_res() };

    // Finalize structure tag
    *static_cast<Tpm_st *>(&cmd) = Tpm_st { session ? Tpm_st::Type::SESSIONS : Tpm_st::Type::NO_SESSIONS };

    if (session)
        session->compute_auth (&cmd, c);

    auto rc { Tpm_rc::Type::FAILURE };

    // Execute on TPM interface
    if (execute (l, cmd)) [[likely]] {

        auto const res { reinterpret_cast<Res const *>(buffer) };

        // Verify response HMAC for successful commands
        if ((rc = res->rc()) == Tpm_rc::Type::SUCCESS) [[likely]]
            return !session || session->verify_auth (res, r, cmd);
    }

    trace (TRACE_ERROR, "TPM2: CMD:%#x failed with ERR:%#x", std::to_underlying (cmd.cc()), std::to_underlying (rc));

    if (session)
        session->undo_nonce();

    return false;
}

bool Tpm::start_auth_session (Locality l, Hmac_session &s)
{
    // Abort if the command failed
    if (!invoke (l, Tpm2_start_auth_session<Hmac_session::Type> { s.older })) [[unlikely]]
        return false;

    // Handle + Nonce
    auto const h { reinterpret_cast<Tpm_handle const *>(buffer + sizeof (Res)) };
    auto const n { reinterpret_cast<Tpm2b_digest<0> const *>(h + 1) };

    // Abort if the TPM returned the wrong nonce size
    if (n->size() != sizeof (s.newer)) [[unlikely]]
        return false;

    // Initialize session nonce
    __builtin_memcpy (s.newer, n->dig, sizeof (s.newer));

    // Initialize session handle
    s.handle = *h;

    return true;
}

bool Tpm::cap_pcrs (Locality l)
{
    // Enumerated capability type
    constexpr auto type { Tpm_cap::Type::PCRS };

    // Number of capabilities that fit in the buffer (assumes 3 bytes for 24 PCRs)
    constexpr auto num { (sizeof (buffer) - sizeof (Res) - sizeof (bool) - sizeof (Tpms_capability_data) - sizeof (Tpml_pcr_selection)) / (sizeof (Tpms_pcr_selection) + 3) };

    // Abort if the command failed
    if (!invoke (l, Tpm2_get_capability { type, Tpm_pt::Type { 0 }, num })) [[unlikely]]
        return false;

    auto const tpms { reinterpret_cast<Tpms_capability_data const *>(buffer + sizeof (Res) + sizeof (bool)) };

    // Abort if the TPM returned the wrong capability type
    if (tpms->cap.type() != type) [[unlikely]]
        return false;

    auto const tpml { tpms->next<Tpml_pcr_selection>() };

    // List items are Tpms_pcr_selection
    auto p { tpml->next() };

    // Enumerate supported hash algorithms that have PCRs allocated to them
    for (auto i { tpml->size() }; i--; p = p->next())
        if (p->sel.pcrs())
            hash.add (p->alg.hash());

    return true;
}

bool Tpm::cap_tpm_properties (Locality l)
{
    // Enumerated capability type
    constexpr auto type { Tpm_cap::Type::TPM_PROPERTIES };

    // Number of capabilities that fit in the buffer
    constexpr auto num { (sizeof (buffer) - sizeof (Res) - sizeof (bool) - sizeof (Tpms_capability_data) - sizeof (Tpml_tagged_tpm_property)) / sizeof (Tpms_tagged_property) };

    // Iterate over all tags in the fixed group
    for (unsigned tag { 0x100 }; tag < 0x200; tag++) {

        // Abort if the command failed
        if (!invoke (l, Tpm2_get_capability { type, Tpm_pt::Type { tag }, num })) [[unlikely]]
            return false;

        auto const tpms { reinterpret_cast<Tpms_capability_data const *>(buffer + sizeof (Res) + sizeof (bool)) };

        // Abort if the TPM returned the wrong capability type
        if (tpms->cap.type() != type) [[unlikely]]
            return false;

        auto const tpml { tpms->next<Tpml_tagged_tpm_property>() };

        // List items are Tpms_tagged_property
        auto p { tpml->next() };

        for (auto i { tpml->size() }; i--; p = p->next()) {

            switch (p->ptg.type()) {
                case Tpm_pt::Type::REVISION:            tpm_rev = p->val; break;
                case Tpm_pt::Type::MANUFACTURER:        __builtin_memcpy (tpm_mfr + 0x0, &p->val, 4); break;
                case Tpm_pt::Type::VENDOR_STRING_1:     __builtin_memcpy (tpm_ven + 0x0, &p->val, 4); break;
                case Tpm_pt::Type::VENDOR_STRING_2:     __builtin_memcpy (tpm_ven + 0x4, &p->val, 4); break;
                case Tpm_pt::Type::VENDOR_STRING_3:     __builtin_memcpy (tpm_ven + 0x8, &p->val, 4); break;
                case Tpm_pt::Type::VENDOR_STRING_4:     __builtin_memcpy (tpm_ven + 0xc, &p->val, 4); break;
                case Tpm_pt::Type::FIRMWARE_VERSION_1:  ver_fw1 = p->val; break;
                case Tpm_pt::Type::FIRMWARE_VERSION_2:  ver_fw2 = p->val; break;
                case Tpm_pt::Type::PCR_COUNT:           num_pcr = p->val; break;
                case Tpm_pt::Type::MAX_COMMAND_SIZE:    max_cmd = p->val; break;
                case Tpm_pt::Type::MAX_RESPONSE_SIZE:   max_res = p->val; break;
            }

            tag = std::to_underlying (p->ptg.type());
        }

        if (!buffer[sizeof (Res)]) [[likely]]
            break;
    }

    return true;
}
