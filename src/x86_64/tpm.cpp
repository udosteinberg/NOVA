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

bool Tpm::init()
{
    if (static_cast<Interface>(read (0, Register32::INTERFACE_ID) & BIT_RANGE (3, 0)) == Interface::CRB)
        return false;

    if (!locality_request (0))
        return false;

    if (!make_ready (0))
        return false;

    if ((read (0, Register32::INTF_CAPABILITY) >> 28 & BIT_RANGE (2, 0)) == 3) {
        fam = static_cast<Family>(read (0, Register32::STATUS) >> 26 & BIT_RANGE (1, 0));
        ifc = Interface::FIFO_PTP;
    } else {
        fam = Family::TPM_12;           // STATUS[31:24] are undefined
        ifc = Interface::FIFO_TIS;
    }

    return locality_release (0);
}

void Tpm::info()
{
    auto const didvid { read (0, Register32::DIDVID) };

    trace (TRACE_DRTM, "DRTM: TPM %04x:%04x (%#x) %s %s",
           static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16), read (0, Register8::RID),
           fam == Family::TPM_20 ? "2.0" : fam == Family::TPM_12 ? "1.2" : "?",
           ifc == Interface::FIFO_PTP ? "PTP" : ifc == Interface::FIFO_TIS ? "TIS" : "?");
}
