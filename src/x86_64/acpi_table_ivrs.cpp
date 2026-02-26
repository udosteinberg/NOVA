/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_table_ivrs.hpp"
#include "hpet.hpp"
#include "ioapic.hpp"
#include "smmu_amd.hpp"

bool Acpi_table_ivrs::Ivhd_11::parse() const
{
    auto const smmu { Smmu_amd::create (phys, Pci::pci (seg, bdf), efr1, efr2) };
    if (!smmu) [[unlikely]]
        panic ("SMMU allocation failed");

    using list_t = Device;
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + len };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { reinterpret_cast<list_t const *>(ptr) };
        auto const l { s->len() };

        trace (TRACE_FIRM | TRACE_PARSE, "IVRS:   Dev:%#04x Len:%u", std::to_underlying (s->type()), l);

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        // Ivhd_11 is defined to only contain fixed-length types
        switch (s->type()) {

            default:
                break;

            case Device::Type::SPECIAL:
                auto const d { static_cast<Device_8 const *>(s) };
                auto const t { Pci::pci (seg, d->src) };
                switch (d->variety) {
                    case 0x1: Ioapic::claim_dev (t, d->handle); break;
                    case 0x2: Hpet::claim_dev (t, d->handle); break;
                }
                break;
        }
    }

    return end == ptr;
}

bool Acpi_table_ivrs::parse() const
{
    using list_t = Block;
    bool       ret { true };
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { reinterpret_cast<list_t const *>(ptr) };
        auto const l { s->len };

        trace (TRACE_FIRM | TRACE_PARSE, "IVRS: Type:%#04x Len:%u", std::to_underlying (s->type()), unsigned { l });

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        switch (s->type()) {

            default:
                break;

            case Block::Type::IVHD_11:
                ret &= static_cast<Ivhd_11 const *>(s)->parse();
                break;
        }
    }

    return ret && end == ptr;
}
