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
#include "ioapic.hpp"
#include "smmu.hpp"

bool Acpi_table_ivrs::Ivhd::parse (size_t hdr, uint64_t, uint64_t) const
{
    // Already enumerated by another IVHD
    if (Smmu::lookup_phys (phys)) [[unlikely]]
        return true;

    using list_t = Device;
    auto       ptr { reinterpret_cast<uintptr_t>(this) + hdr };
    auto const end { reinterpret_cast<uintptr_t>(this) + len };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };
        auto const l { s->len (end) };

        trace (TRACE_FIRM | TRACE_PARSE, "IVRS:   Dev:%#04x Len:%u", std::to_underlying (s->type()), l);

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        switch (s->type()) {

            default:
                break;

            case Device::Type::SPECIAL:
                auto const d { std::start_lifetime_as<Device_8 const> (s) };
                auto const t { Pci::pci (seg, d->src) };
                switch (d->variety) {
                    case 0x1: Ioapic::claim_dev (t, d->handle); break;
                }
                break;
        }
    }

    return end == ptr;
}

void Acpi_table_ivrs::parse_entry (Ivdb::Type const t, uintptr_t ptr, uintptr_t const end, bool &ret)
{
    using list_t = Ivdb;

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };
        auto const l { s->len };

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        // Skip entries that don't match the requested type
        if (s->type() != t) [[likely]]
            continue;

        trace (TRACE_FIRM | TRACE_PARSE, "IVRS: Type:%#04x Len:%u", std::to_underlying (s->type()), unsigned { l });

        switch (t) {

            case Ivdb::Type::IVHD_10:
                ret &= std::start_lifetime_as<Ivhd_10 const> (s)->parse();
                break;

            case Ivdb::Type::IVHD_11:
            case Ivdb::Type::IVHD_40:
                ret &= std::start_lifetime_as<Ivhd_11 const> (s)->parse();
                break;

            default:
                break;
        }
    }

    ret &= end == ptr;
}

bool Acpi_table_ivrs::parse() const
{
    auto const ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    bool ret { true };

    /*
     * Newer types take precedence over older types.
     * IVHD type 40h and 11h are reserved when IVinfo[EFRSup]=0.
     */
    if (ivinfo & BIT (0)) [[likely]] {
        parse_entry (Ivdb::Type::IVHD_40, ptr, end, ret);
        parse_entry (Ivdb::Type::IVHD_11, ptr, end, ret);
    }

    parse_entry (Ivdb::Type::IVHD_10, ptr, end, ret);

    return ret;
}
