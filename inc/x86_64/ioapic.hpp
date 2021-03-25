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

#include "intid.hpp"
#include "lock_guard.hpp"
#include "mmio.hpp"
#include "pci.hpp"
#include "vectors.hpp"

class Ioapic final : public List<Ioapic>, private Mmio
{
    private:
        unsigned    const       gsi_base;
        unsigned    const       gsi_last;
        uint8_t     const       id;                                         // Enumeration ID
        pci_t                   pci         { 0 };                          // PCI S:B:D:F
        Spinlock                lock;

        static Slab_cache cache;                                            // IOAPIC Slab Cache
        static inline constinit Ioapic *  list { nullptr };                 // IOAPIC Device List

        // Direct Registers
        enum class Reg32 : unsigned
        {
            IDX     = 0x00,                 // rw Index Register
            DAT     = 0x10,                 // rw Data Register
            PAR     = 0x20,                 // -w Pin Assertion Register
            EOI     = 0x40,                 // -w EOI Register
        };

        // Indirect Registers
        enum class Ind32 : unsigned
        {
            ID      = 0x00,                 // rw ID Register
            VER     = 0x01,                 // r- Version Register
            ARB     = 0x02,                 // r- Arbitration ID Register
            BCFG    = 0x03,                 // rw Boot Configuration Register
            RTE     = 0x10,                 // rw Redirection Table Entry Register
        };

        auto read  (Reg32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)); }
        void write (Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmio + std::to_underlying (r)) = v; }

        void index (Ind32 r) const
        {
            write (Reg32::IDX, std::to_underlying (r));
        }

        uint32_t read (Ind32 r)
        {
            Lock_guard <Spinlock> guard { lock };
            index (r);
            return read (Reg32::DAT);
        }

        void write (Ind32 r, uint32_t v)
        {
            Lock_guard <Spinlock> guard { lock };
            index (r);
            write (Reg32::DAT, v);
        }

        void init();

    public:
        auto src() const { return Pci::bdf (pci); }
        auto ver() { return static_cast<uint8_t>(read (Ind32::VER)); }
        auto mre() { return static_cast<uint8_t>(read (Ind32::VER) >> 16); }

        explicit Ioapic (uint64_t p, uint8_t i, unsigned g) : List { list }, Mmio { p, PAGE_SIZE (0), Memattr::dev() }, gsi_base { g }, gsi_last { g + mre() }, id { i } {}

        void eoi (uint8_t vec)
        {
            // Check vec range
            assert (vec >= VEC_GSI && vec < VEC_LVT);

            // Acknowledge
            write (Reg32::EOI, vec);
        }

        void set_dst (unsigned gsi, uint32_t v)
        {
            // Check gsi range
            assert (gsi >= gsi_base && gsi <= gsi_last);

            // Determine redirection table entry
            auto const rte { gsi - gsi_base };

            // Update
            write (Ind32 (std::to_underlying (Ind32::RTE) + 2 * rte + 1), v);
        }

        void set_cfg (unsigned gsi, uint8_t vec = 0, bool msk = true, bool trg = false, bool pol = false)
        {
            // Check gsi range
            assert (gsi >= gsi_base && gsi <= gsi_last);

            // Check vec range
            assert (msk || (vec >= VEC_GSI && vec < VEC_LVT));

            // Determine redirection table entry
            auto const rte { gsi - gsi_base };

            // Update
            write (Ind32 (std::to_underlying (Ind32::RTE) + 2 * rte), msk << 16 | trg << 15 | pol << 13 | vec);
        }

        /*
         * Lookup IOAPIC to which IID is connected
         *
         * @param i     Interrupt ID
         * @return      Pointer to IOAPIC (PIN) or nullptr (MSI)
         */
        static Ioapic *lookup (iid_t i)
        {
            auto const seg { Intid::to_seg (i) };
            auto const gsi { Intid::to_gsi (i) };

            for (auto l { list }; l; l = l->next)
                if (seg == Pci::seg (l->pci) && gsi >= l->gsi_base && gsi <= l->gsi_last)
                    return l;

            return nullptr;
        }

        static bool claim_dev (pci_t p, uint8_t i)
        {
            for (auto l { list }; l; l = l->next)
                if (l->id == i) {
                    l->pci = p;
                    return true;
                }

            return false;
        }

        static void init_all()
        {
            for (auto l { list }; l; l = l->next)
                l->init();
        }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }
};
