/*
 * Flattened Devicetree (FDT)
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

#include "assert.hpp"
#include "fdt.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"
#include "string.hpp"
#include "uefi.hpp"

bool Fdt::Header::parse (uint64_t phys) const
{
    if (get_magic() != 0xd00dfeed)
        return false;

    trace (TRACE_FIRM, "FDTB: %#010lx Version:%u Size:%#x BootCPU:%u", phys, get_fdt_version(), get_fdt_size(), get_boot_cpu());

    fdtb = reinterpret_cast<decltype (fdtb)>(reinterpret_cast<uintptr_t>(this) + get_offs_structs());
    fdte = reinterpret_cast<decltype (fdte)>(reinterpret_cast<uintptr_t>(fdtb) + get_size_structs());
    fdts = reinterpret_cast<decltype (fdts)>(reinterpret_cast<uintptr_t>(this) + get_offs_strings());

    parse_subtree (fdtb, -1U, -1U, 0);

    return true;
}

void Fdt::Header::parse_subtree (be_uint32_t const *&w, unsigned pa_cells, unsigned ps_cells, unsigned l) const
{
    unsigned const indent { 4 };
    unsigned a_cells { -1U }, s_cells { -1U };

    uint32_t l_interrupts { 0 }; be_uint32_t const *p_interrupts { nullptr };
    uint32_t l_ranges     { 0 }; be_uint32_t const *p_ranges     { nullptr };
    uint32_t l_reg        { 0 }; be_uint32_t const *p_reg        { nullptr };

    while (w < fdte) {

        switch (*w) {

            case FDT_BEGIN_NODE:
                {
                    auto p { reinterpret_cast<char const *>(w + 1) };
                    trace (TRACE_FIRM | TRACE_PARSE, "%*s%s{", l * indent, "", p);
                    while (*p++) ;
                    w = reinterpret_cast<be_uint32_t const *>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);
                    parse_subtree (w, a_cells, s_cells, l + 1);
                    continue;
                }

            case FDT_END_NODE:
                if (p_interrupts) {
                    auto v { p_interrupts };
                    unsigned const ic { 3 };
                //  assert (l_interrupts % (ic * sizeof (uint32_t)) == 0);
                    for (auto i { l_interrupts / sizeof (uint32_t) / ic }; i--; ) {
                        uint32_t t { *v++ }, n { *v++ }, f { *v++ };
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#x/%#x/%#x", l * indent, "", "interrupts", t, n, f);
                    }
                }

                if (p_ranges) {
                    auto v { p_ranges };
                    unsigned const cc {  a_cells != -1U ?  a_cells : 2 };
                    unsigned const pc { pa_cells != -1U ? pa_cells : 2 };
                    unsigned const sc {  s_cells != -1U ?  s_cells : 1 };
                    assert (l_ranges % ((cc + pc + sc) * sizeof (uint32_t)) == 0);
                    for (auto i { l_ranges / sizeof (uint32_t) / (cc + pc + sc) }; i--; ) {
                        uint64_t cbus { 0 }, pbus { 0 }, size { 0 };
                        for (auto j { cc }; j--; cbus = (cbus << 32) | *v++) ;
                        for (auto j { pc }; j--; pbus = (pbus << 32) | *v++) ;
                        for (auto j { sc }; j--; size = (size << 32) | *v++) ;
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#lx/%#lx/%#lx", l * indent, "", "ranges", cbus, pbus, size);
                    }
                }

                if (p_reg) {
                    auto v { p_reg };
                    unsigned const ac { pa_cells != -1U ? pa_cells : 2 };
                    unsigned const sc { ps_cells != -1U ? ps_cells : 1 };
                    assert (l_reg % ((ac + sc) * sizeof (uint32_t)) == 0);
                    for (auto i { l_reg / sizeof (uint32_t) / (ac + sc) }; i--; ) {
                        uint64_t addr { 0 }, size { 0 };
                        for (auto j { ac }; j--; addr = (addr << 32) | *v++) ;
                        for (auto j { sc }; j--; size = (size << 32) | *v++) ;
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#lx/%#lx", l * indent, "", "reg", addr, size);
                    }
                }

                trace (TRACE_FIRM | TRACE_PARSE, "%*s}", --l * indent, "");
                w++;
                return;

            case FDT_PROP:
                {
                    auto const len { *(w + 1) };
                    auto const s { *(w + 2) + fdts };
                    auto v { w + 3 };
                    auto const p { reinterpret_cast<char const *>(v) };

                    if (!len) {
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = true", l * indent, "", s);

                    // u32 Single
                    } else if (*s == '#' || !strcmp (s, "interrupt-parent") || !strcmp (s, "phandle") || !strcmp (s, "virtual-reg")) {
                        uint32_t const val { *v };
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#x", l * indent, "", s, val);
                        if (!strcmp (s, "#address-cells"))
                            a_cells = val;
                        else if (!strcmp (s, "#size-cells"))
                            s_cells = val;

                    // u32 Multiple
                    } else if (!strcmp (s, "clocks")) {
                        for (auto i { len / sizeof (uint32_t) }; i--; v++) {
                            uint32_t const val { *v };
                            trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#x", l * indent, "", s, val);
                        }

                    // u32 or u64
                    } else if (!strcmp (s, "clock-frequency") || !strcmp (s, "timebase-frequency")) {
                        uint64_t val { 0 };
                        for (auto i { len / sizeof (uint32_t) }; i--; val = (val << 32) | *v++) ;
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %lu", l * indent, "", s, val);

                    // String
                    } else if (!strcmp (s, "device_type") || !strcmp (s, "model") || !strcmp (s, "name") || !strcmp (s, "status")) {
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %s", l * indent, "", s, p);

                    // Stringlist
                    } else if (!strcmp (s, "clock-names") || !strcmp (s, "compatible") || !strcmp (s, "enable-method")) {
                        char const *x, *y;
                        for (x = y = p; y < p + len; y++)
                            if (!*y) {
                                trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %s", l * indent, "", s, x);
                                x = y + 1;
                            }

                    // Prop-Encoded Array
                    } else if (!strcmp (s, "interrupts")) {
                        l_interrupts = len;
                        p_interrupts = v;

                    // Prop-Encoded Array
                    } else if (!strcmp (s, "ranges")) {
                        l_ranges = len;
                        p_ranges = v;

                    // Prop-Encoded Array
                    } else if (!strcmp (s, "reg")) {
                        l_reg = len;
                        p_reg = v;

                    // String?
                    } else
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s? = %s", l * indent, "", s, p);

                    w = reinterpret_cast<be_uint32_t const *>((reinterpret_cast<uintptr_t>(w + 3) + len + 3) & ~3);
                    continue;
                }

            default:
                w++;
                continue;
        }
    }
}

bool Fdt::init()
{
    auto const p { Uefi::info.fdtp };

    // No FDT or FDT not properly aligned
    if (!p || p & 7)
        return false;

    return static_cast<Fdt::Header *>(Hptp::map (MMAP_GLB_MAP0, p))->parse (p);
}
