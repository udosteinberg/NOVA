/*
 * Flattened Devicetree (FDT)
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

#include "assert.hpp"
#include "cpu.hpp"
#include "extern.hpp"
#include "fdt.hpp"
#include "psci.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"
#include "string.hpp"
#include "uefi.hpp"

uint32_t const *  Fdt::fdtb { nullptr };
uint32_t const *  Fdt::fdte { nullptr };
char     const *  Fdt::fdts { nullptr };

bool Fdt::parse (uint64_t phys) const
{
    if (!valid())
        return false;

    trace (TRACE_FIRM, "FDTB: %#010lx Version:%u Size:%#x BootCPU:%u", phys, fdt_version(), fdt_size(), boot_cpu());

    fdtb = reinterpret_cast<decltype (fdtb)>(reinterpret_cast<uintptr_t>(this) + offs_structs());
    fdte = reinterpret_cast<decltype (fdte)>(reinterpret_cast<uintptr_t>(fdtb) + size_structs());
    fdts = reinterpret_cast<decltype (fdts)>(reinterpret_cast<uintptr_t>(this) + offs_strings());

    parse_subtree (fdtb, -1U, -1U, 0);

    return true;
}

void Fdt::parse_subtree (uint32_t const *&w, unsigned pa_cells, unsigned ps_cells, unsigned l) const
{
    unsigned const indent = 4;
    unsigned a_cells = -1U, s_cells = -1U;

    uint32_t l_interrupts = 0; uint32_t const *p_interrupts = nullptr;
    uint32_t l_ranges     = 0; uint32_t const *p_ranges     = nullptr;
    uint32_t l_reg        = 0; uint32_t const *p_reg        = nullptr;

    while (w < fdte) {

        switch (convert (*w)) {

            case FDT_BEGIN_NODE:
                {
                    char const *p = reinterpret_cast<char const *>(w + 1);
                    trace (TRACE_FIRM | TRACE_PARSE, "%*s%s{", l * indent, "", p);
                    while (*p++) ;
                    w = reinterpret_cast<uint32_t const *>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);
                    parse_subtree (w, a_cells, s_cells, l + 1);
                    continue;
                }

            case FDT_END_NODE:
                if (p_interrupts) {
                    uint32_t const *v = p_interrupts;
                    unsigned ic = 3;
                //  assert (l_interrupts % (ic * sizeof (uint32_t)) == 0);
                    for (unsigned i = 0; i < l_interrupts / sizeof (uint32_t) / ic; i++, v += 3)
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#x/%#x/%#x", l * indent, "", "interrupts", convert (*v), convert (*(v + 1)), convert (*(v + 2)));
                }

                if (p_ranges) {
                    uint32_t const *v = p_ranges;
                    unsigned cc =  a_cells != -1U ?  a_cells : 2;
                    unsigned pc = pa_cells != -1U ? pa_cells : 2;
                    unsigned sc =  s_cells != -1U ?  s_cells : 1;
                    assert (l_ranges % ((cc + pc + sc) * sizeof (uint32_t)) == 0);
                    for (unsigned i = 0; i < l_ranges / sizeof (uint32_t) / (cc + pc + sc); i++) {
                        uint64_t cbus = 0, pbus = 0, size = 0;
                        for (unsigned j = 0; j < cc; j++, v++)
                            cbus = (cbus << 32) | convert (*v);
                        for (unsigned j = 0; j < pc; j++, v++)
                            pbus = (pbus << 32) | convert (*v);
                        for (unsigned j = 0; j < sc; j++, v++)
                            size = (size << 32) | convert (*v);
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#lx/%#lx/%#lx", l * indent, "", "ranges", cbus, pbus, size);
                    }
                }

                if (p_reg) {
                    uint32_t const *v = p_reg;
                    unsigned ac = pa_cells != -1U ? pa_cells : 2;
                    unsigned sc = ps_cells != -1U ? ps_cells : 1;
                    assert (l_reg % ((ac + sc) * sizeof (uint32_t)) == 0);
                    for (unsigned i = 0; i < l_reg / sizeof (uint32_t) / (ac + sc); i++) {
                        uint64_t addr = 0, size = 0;
                        for (unsigned j = 0; j < ac; j++, v++)
                            addr = (addr << 32) | convert (*v);
                        for (unsigned j = 0; j < sc; j++, v++)
                            size = (size << 32) | convert (*v);
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#lx/%#lx", l * indent, "", "reg", addr, size);
                    }
                }

                trace (TRACE_FIRM | TRACE_PARSE, "%*s}", --l * indent, "");
                w++;
                return;

            case FDT_PROP:
                {
                    uint32_t const len = convert (*(w + 1)), *v = w + 3;
                    char const *s = fdts + convert (*(w + 2)), *p = reinterpret_cast<char const *>(v);

                    if (!len) {
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = true", l * indent, "", s);

                    // u32 Single
                    } else if (*s == '#' || !strcmp (s, "interrupt-parent") || !strcmp (s, "phandle") || !strcmp (s, "virtual-reg")) {
                        uint32_t val = convert (*v);
                        trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#x", l * indent, "", s, val);
                        if (!strcmp (s, "#address-cells"))
                            a_cells = val;
                        else if (!strcmp (s, "#size-cells"))
                            s_cells = val;

                    // u32 Multiple
                    } else if (!strcmp (s, "clocks")) {
                        for (unsigned i = 0; i < len / sizeof (uint32_t); i++, v++)
                            trace (TRACE_FIRM | TRACE_PARSE, "%*s%s = %#x", l * indent, "", s, convert (*v));

                    // u32 or u64
                    } else if (!strcmp (s, "clock-frequency") || !strcmp (s, "timebase-frequency")) {
                        uint64_t val = 0;
                        for (unsigned i = 0; i < len / sizeof (uint32_t); i++, v++)
                            val = (val << 32) | convert (*v);
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

                    p += len;
                    w = reinterpret_cast<uint32_t const *>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);
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
    unsigned const cnt { sizeof (Board::cpu) / sizeof (*Board::cpu) };

    if (Board::spin_addr) {

        auto spintable { reinterpret_cast<uint64_t *>(Hptp::map (Hptp::Remap::MAP0, Board::spin_addr, true)) };

        for (unsigned i { 0 }; i < cnt; i++) {
            spintable[i] = Kmem::sym_to_phys (&__init_spin);
            asm volatile ("dsb ish; dc cvac, %0; sev" : : "r" (spintable + i) : "memory");
        }

        Cpu::count = cnt;

    } else {

        Psci::init();

        for (cpu_t c { 0 }; c < cnt; c++)
            if (Psci::boot_cpu (c, Board::cpu[c].id))
                Cpu::count++;
    }

    auto const p { reinterpret_cast<Uefi *>(Kmem::sym_to_virt (&Uefi::uefi))->fdtp };

    // No FDT or FDT not properly aligned
    if (!p || p & 7)
        return false;

    return static_cast<Fdt *>(Hptp::map (Hptp::Remap::MAP0, p))->parse (p);
}
