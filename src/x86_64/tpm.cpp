/*
 * Trusted Platform Module (TPM)
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
                v1_cap_tpm_properties (num_pcr, Tpm1_ptg::Type::PTG_PCR_COUNT);
                v1_cap_tpm_properties (tpm_mfr, Tpm1_ptg::Type::PTG_MANUFACTURER);
                v1_cap_tpm_properties (max_buf, Tpm1_ptg::Type::PTG_INPUT_BUFFER);
                hash.add (Tpm_ai::Type::SHA1_160);
                break;

            case Family::TPM_20:
                v2_cap_tpm_properties();
                v2_cap_pcrs();
                break;
        }

        auto const vendor { __builtin_bswap32 (tpm_mfr) };

        trace (TRACE_TPM, "TPM%u: %04x:%04x (%.4s) %s %s ALG:%u PCR:%u BUF:%u", std::to_underlying (fam) + 1,
               static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16), reinterpret_cast<char const *>(&vendor),
               ift == Iftype::FIFO ? "FIFO" : ift == Iftype::CRB ? "CRB" : "?",
               fam == Family::TPM_20 ? "2.0" : fam == Family::TPM_12 ? "1.2" : "?",
               hash.count(), num_pcr, max_buf);
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
        if (!b && !burstcount (l, b)) [[unlikely]]
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
        if (!b && !burstcount (l, b)) [[unlikely]]
            return false;

        *p++ = read (l, Reg8::FIFO_DATA);

        // Update response size
        if (p - buffer == sizeof (Res)) [[unlikely]]
            s = min (sizeof (buffer), static_cast<size_t>(reinterpret_cast<Res *>(buffer)->size()));
    }

    // Check for complete transmission
    return wait_done (l, BIT (4));
}

bool Tpm::Fifo::execute (Locality l, Cmd const &cmd)
{
    // Enter ready state
    if (!state (l, false)) [[unlikely]]
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

        *p++ = read (l, Reg8 { std::to_underlying (Reg8::CRB_DATA) + i });

        // Update response size
        if (p - buffer == sizeof (Res)) [[unlikely]]
            s = min (sizeof (buffer), static_cast<size_t>(reinterpret_cast<Res *>(buffer)->size()));
    }

    return true;
}

bool Tpm::Crb::execute (Locality l, Cmd const &cmd)
{
    // Enter ready state
    if (!state (l, BIT (0))) [[unlikely]]
        return false;

    bool const ret { send (l, cmd) && exec (l) && recv (l) };

    // Enter idle state
    return state (l, BIT (1)) && ret;
}

bool Tpm::execute (Locality l, Cmd const &cmd)
{
    if (!(ift == Iftype::CRB ? Crb::execute (l, cmd) : Fifo::execute (l, cmd))) [[unlikely]]
        return false;

    auto const r { reinterpret_cast<Res const *>(buffer) };

    // Success
    if (r->type() == Tpm_rc::Type::RC_SUCCESS) [[likely]]
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
    constexpr auto num { (sizeof (buffer) - sizeof (Res) - sizeof (bool) - sizeof (Tpms_capability_data) - sizeof (Tpml_pcr_selection)) / (sizeof (Tpms_pcr_selection) + 3) };

    // Abort if the command failed
    if (!execute (Locality::L0, Tpm2_get_capability { Tpm2_cap::Type::CAP_PCRS, Tpm2_ptg::Type { 0 }, num })) [[unlikely]]
        return false;

    auto const tpms { reinterpret_cast<Tpms_capability_data const *>(buffer + sizeof (Res) + sizeof (bool)) };

    // Abort if the command returned the wrong capability type
    if (tpms->cap.type() != Tpm2_cap::Type::CAP_PCRS) [[unlikely]]
        return false;

    auto const tpml { tpms->next<Tpml_pcr_selection>() };

    // List items are Tpms_pcr_selection
    auto p { tpml->next() };

    // Enumerate supported hash algorithms that have PCRs allocated to them
    for (auto i { tpml->size() }; i--; p = p->next())
        if (p->sel.pcrs())
            hash.add (p->alg.type());

    return true;
}

bool Tpm::v2_cap_tpm_properties()
{
    // Number of capabilities that fit in the buffer
    constexpr auto num { (sizeof (buffer) - sizeof (Res) - sizeof (bool) - sizeof (Tpms_capability_data) - sizeof (Tpml_tagged_tpm_property)) / sizeof (Tpms_tagged_property) };

    // Iterate over all tags in the fixed group
    for (unsigned tag { 0x100 }; tag < 0x200; tag++) {

        // Abort if the command failed
        if (!execute (Locality::L0, Tpm2_get_capability { Tpm2_cap::Type::CAP_TPM_PROPERTIES, Tpm2_ptg::Type { tag }, num })) [[unlikely]]
            return false;

        auto const tpms { reinterpret_cast<Tpms_capability_data const *>(buffer + sizeof (Res) + sizeof (bool)) };

        // Abort if the command returned the wrong capability type
        if (tpms->cap.type() != Tpm2_cap::Type::CAP_TPM_PROPERTIES) [[unlikely]]
            return false;

        auto const tpml { tpms->next<Tpml_tagged_tpm_property>() };

        // List items are Tpms_tagged_property
        auto p { tpml->next() };

        for (auto i { tpml->size() }; i--; p = p->next()) {

            switch (p->ptg.type()) {
                case Tpm2_ptg::Type::PTG_MANUFACTURER: tpm_mfr = p->val; break;
                case Tpm2_ptg::Type::PTG_INPUT_BUFFER: max_buf = p->val; break;
                case Tpm2_ptg::Type::PTG_PCR_COUNT:    num_pcr = p->val; break;
            }

            tag = std::to_underlying (p->ptg.type());
        }

        if (!buffer[sizeof (Res)]) [[likely]]
            break;
    }

    return true;
}

bool Tpm::v1_cap_tpm_properties (uint32_t &val, Tpm1_ptg::Type t)
{
    auto const ptr { reinterpret_cast<Unaligned_be<uint32_t> const *>(buffer + sizeof (Res)) };

    // Abort if the command failed or returned the wrong size
    if (!execute (Locality::L0, Tpm1_get_capability { t }) || *ptr != sizeof (uint32_t)) [[unlikely]]
        return false;

    val = *(ptr + 1);

    return true;
}
