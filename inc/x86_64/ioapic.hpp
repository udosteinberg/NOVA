/*
 * I/O Advanced Programmable Interrupt Controller (IOAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "lock_guard.hpp"
#include "mmio.hpp"
#include "pci.hpp"

class alignas (256) Ioapic final : public List<Ioapic>, private Mmio
{
    private:
        enum class Reg32 : unsigned         // Direct Registers
        {
            IDX     = 0x00,                 // rw Index Register
            DAT     = 0x10,                 // rw Data Register
            PAR     = 0x20,                 // -w Pin Assertion Register
            EOI     = 0x40,                 // -w EOI Register
        };

        enum class Ind32 : unsigned         // Indirect Registers
        {
            ID      = 0x00,                 // rw ID Register
            VER     = 0x01,                 // r- Version Register
            ARB     = 0x02,                 // r- Arbitration ID Register
            BCFG    = 0x03,                 // rw Boot Configuration Register
            RTE     = 0x10,                 // rw Redirection Table Entry Register
        };

        auto read  (Reg32 r) const      { return *std::start_lifetime_as<uint32_t volatile> (mmio + std::to_underlying (r)); }
        void write (Reg32 r, uint32_t v) const { *std::start_lifetime_as<uint32_t volatile> (mmio + std::to_underlying (r)) = v; }

        // @pre caller must hold lock
        auto ind_read (Ind32 r) const
        {
            write (Reg32::IDX, std::to_underlying (r));

            return read (Reg32::DAT);
        }

        // @pre caller must hold lock
        void ind_write (Ind32 r, uint32_t v) const
        {
            write (Reg32::IDX, std::to_underlying (r));

            write (Reg32::DAT, v);
        }

        auto read (Ind32 r) const
        {
            Lock_guard <Spinlock> guard { lock };

            return ind_read (r);
        }

        void write (Ind32 r, uint32_t v) const
        {
            Lock_guard <Spinlock> guard { lock };

            ind_write (r, v);
        }

        auto rte_read (unsigned pin) const
        {
            auto const idx { std::to_underlying (Ind32::RTE) + 2 * pin };

            Lock_guard <Spinlock> guard { lock };

            auto const hi { ind_read (Ind32 { idx + 1 }) };
            auto const lo { ind_read (Ind32 { idx + 0 }) };

            return uint64_t { hi } << 32 | lo;
        }

        void rte_write (unsigned pin, uint64_t v) const
        {
            auto const idx { std::to_underlying (Ind32::RTE) + 2 * pin };

            Lock_guard <Spinlock> guard { lock };

            // If RTE is level-triggered, then force RIRR=0 via transient masked/edge configuration
            if (v & BIT (15)) [[unlikely]]
                ind_write (Ind32 { idx }, BIT (16));

            ind_write (Ind32 { idx + 1 }, static_cast<uint32_t>(v >> 32));
            ind_write (Ind32 { idx + 0 }, static_cast<uint32_t>(v));
        }

        void rte_mask (unsigned pin) const
        {
            auto const idx { std::to_underlying (Ind32::RTE) + 2 * pin };

            Lock_guard <Spinlock> guard { lock };

            ind_write (Ind32 { idx }, BIT (16));
        }

        void init() const;

        Spinlock       lock;                        // Indirect Register Lock
        pci_t          sbdf;                        // PCI S:B:D:F
        unsigned const gsi_base;
        unsigned const gsi_last;
        uint8_t  const id;                          // Enumeration ID

        static Slab_cache cache;                    // IOAPIC Slab Cache
        static inline constinit Ioapic *list {};    // IOAPIC Device List

    public:
        auto src() const { return sbdf; }
        auto ver() const { return static_cast<uint8_t>(read (Ind32::VER)); }
        auto mre() const { return static_cast<uint8_t>(read (Ind32::VER) >> 16); }

        explicit Ioapic (uint64_t p, uint8_t i, unsigned g) : List { list }, Mmio { p, PAGE_SIZE (0), Memattr::dev() }, sbdf { 0 }, gsi_base { g }, gsi_last { g + mre() }, id { i } {}

        void eoi (uint8_t val) const { write (Reg32::EOI, val); }

        auto get_pin (unsigned gsi) const { return gsi - gsi_base; }

        void rte_clr_compat (unsigned gsi, uint8_t dst, uint8_t vec) const
        {
            auto const pin { get_pin (gsi) };
            auto const rte { rte_read (pin) };

            // Zap RTE (only if DST:VEC is the live sink)
            if (static_cast<uint8_t>(rte >> 56) == dst && static_cast<uint8_t>(rte) == vec)
                rte_mask (pin);
        }

        void rte_set_compat (Atomic<uintptr_t> &ise, unsigned gsi, bool msk, bool trg, bool pol, uint8_t dst, uint8_t vec) const
        {
            assert (msk || (vec >= 0x10 && vec <= 0xfe));

            auto const pin { get_pin (gsi) };

            // Update interrupt source encoding before the interrupt can fire
            ise = trg ? reinterpret_cast<uintptr_t>(this) | vec : 0;

            rte_write (pin, uint64_t { dst } << 56 | msk << 16 | trg << 15 | pol << 13 | vec);
        }

        void rte_set_ir_amd (Atomic<uintptr_t> &ise, unsigned gsi, bool msk, bool trg, bool pol) const
        {
            auto const pin { get_pin (gsi) };

            // Update interrupt source encoding before the interrupt can fire
            ise = trg ? reinterpret_cast<uintptr_t>(this) | pin : 0;

            rte_write (pin, msk << 16 | trg << 15 | pol << 13 | pin);
        }

        void rte_set_ir_itl (Atomic<uintptr_t> &ise, unsigned gsi, bool msk, bool trg, bool pol) const
        {
            auto const pin { get_pin (gsi) };

            // Update interrupt source encoding before the interrupt can fire
            ise = trg ? reinterpret_cast<uintptr_t>(this) | pin : 0;

            rte_write (pin, uint64_t { gsi & BIT_RANGE (14, 0) } << 49 | BIT64 (48) | msk << 16 | trg << 15 | pol << 13 | (gsi & BIT (15)) >> 4 | pin);
        }

        static Ioapic const *lookup (uint16_t seg, uint16_t gsi)
        {
            for (auto ioapic { list }; ioapic; ioapic = ioapic->next)
                if (seg == Pci::seg (ioapic->sbdf) && gsi >= ioapic->gsi_base && gsi <= ioapic->gsi_last)
                    return ioapic;

            return nullptr;
        }

        static bool claim_dev (pci_t sbdf, uint8_t id)
        {
            for (auto ioapic { list }; ioapic; ioapic = ioapic->next)
                if (ioapic->id == id) {
                    ioapic->sbdf = sbdf;
                    return true;
                }

            return false;
        }

        /*
         * Initialize all IOAPICs
         */
        static void initialize()
        {
            for (auto ioapic { list }; ioapic; ioapic = ioapic->next)
                ioapic->init();
        }

        [[nodiscard]] static void *operator new (size_t) noexcept { return cache.alloc(); }
};

static_assert (alignof (Ioapic) == 256);
