/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "io.hpp"
#include "list.hpp"
#include "memory.hpp"
#include "slab.hpp"
#include "std.hpp"

class Smmu;

class Pci final : public List<Pci>
{
    friend class Acpi_table_mcfg;

    public:
        enum class Register8 : unsigned
        {
            HTYPE   = 0xe,      // Header Type
            BUS_PRI = 0x18,     // Bus Number: Primary
            BUS_SEC = 0x19,     // Bus Number: Secondary
            BUS_SUB = 0x1a,     // Bus Number: Subordinate
        };

        enum class Register16 : unsigned
        {
            VID     = 0x0,      // Vendor ID
            DID     = 0x2,      // Device ID
            CMD     = 0x4,      // Command Register
            STS     = 0x6,      // Status Register
        };

        enum class Register32 : unsigned
        {
            DID_VID = 0x0,      // Device ID + Vendor ID
            STS_CMD = 0x4,      // Status Register + Command Register
            CCP_RID = 0x8,      // Class Codes + Programming Interface + Revision ID
            BAR0    = 0x10,     // Base Address Register 0
            BAR1    = 0x14,     // Base Address Register 1
        };

        // Compose B:D:F triplet
        static constexpr auto bdf (uint8_t b, uint8_t d, uint8_t f) { return static_cast<pci_t>(b << 8 | d << 3 | f); }

        // Decompose B:D:F triplet
        static constexpr auto bus (pci_t bdf) { return static_cast<uint8_t>(bdf >> 8); }
        static constexpr auto dev (pci_t bdf) { return static_cast<uint8_t>(BIT_RANGE (4, 0) & bdf >> 3); }
        static constexpr auto fun (pci_t bdf) { return static_cast<uint8_t>(BIT_RANGE (2, 0) & bdf); }

        // Configuration Space Address
        static void cfg_addr (pci_t bdf, unsigned r) { Io::out<uint32_t>(0xcf8, BIT (31) | bdf << 8 | r); }

        // Configuration Space Data Port
        static auto cfg_data (Register8  r) { return static_cast<port_t>(0xcfc | (std::to_underlying (r) & 3)); }
        static auto cfg_data (Register16 r) { return static_cast<port_t>(0xcfc | (std::to_underlying (r) & 2)); }
        static auto cfg_data (Register32)   { return static_cast<port_t>(0xcfc); }

        // Configuration Space Read
        static auto cfg_read (pci_t bdf, Register8  r) { cfg_addr (bdf, std::to_underlying (r)); return Io::in<uint8_t> (cfg_data (r)); }
        static auto cfg_read (pci_t bdf, Register16 r) { cfg_addr (bdf, std::to_underlying (r)); return Io::in<uint16_t>(cfg_data (r)); }
        static auto cfg_read (pci_t bdf, Register32 r) { cfg_addr (bdf, std::to_underlying (r)); return Io::in<uint32_t>(cfg_data (r)); }

        // Configuration Space Write
        static void cfg_write (pci_t bdf, Register8  r, uint8_t  v) { cfg_addr (bdf, std::to_underlying (r)); Io::out<uint8_t> (cfg_data (r), v); }
        static void cfg_write (pci_t bdf, Register16 r, uint16_t v) { cfg_addr (bdf, std::to_underlying (r)); Io::out<uint16_t>(cfg_data (r), v); }
        static void cfg_write (pci_t bdf, Register32 r, uint32_t v) { cfg_addr (bdf, std::to_underlying (r)); Io::out<uint32_t>(cfg_data (r), v); }

        // Configuration Space Read/Write
        static void cfg_rd_wr (pci_t bdf, Register8  r, uint8_t  s) { cfg_addr (bdf, std::to_underlying (r)); Io::out<uint8_t> (cfg_data (r), Io::in<uint8_t> (cfg_data (r)) | s); }
        static void cfg_rd_wr (pci_t bdf, Register16 r, uint16_t s) { cfg_addr (bdf, std::to_underlying (r)); Io::out<uint16_t>(cfg_data (r), Io::in<uint16_t>(cfg_data (r)) | s); }
        static void cfg_rd_wr (pci_t bdf, Register32 r, uint32_t s) { cfg_addr (bdf, std::to_underlying (r)); Io::out<uint32_t>(cfg_data (r), Io::in<uint32_t>(cfg_data (r)) | s); }

        Pci (pci_t, uint16_t);

        static uint8_t init (uint8_t, uint8_t, uint16_t = 0);

        static bool match_list (uint16_t id, uint16_t const list[])
        {
            for (auto ptr { list }; *ptr; ptr++)
                if (*ptr == id)
                    return true;

            return false;
        }

        static void claim_all (Smmu *s)
        {
            for (auto l { list }; l; l = l->next)
                if (!l->smmu)
                    l->smmu = s;
        }

        static bool claim_dev (Smmu *s, pci_t d)
        {
            auto pci { find_dev (d) };

            if (!pci)
                return false;

            auto const l { pci->lev };

            do pci->smmu = s; while ((pci = pci->next) && pci->lev > l);

            return true;
        }

        static Smmu *find_smmu (pci_t d)
        {
            auto pci { find_dev (d) };

            return pci ? pci->smmu : nullptr;
        }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    private:
        pci_t       const   sid;
        uint16_t    const   lev;
        Smmu *              smmu            { nullptr };

        static          Slab_cache  cache;                      // PCI Slab Cache
        static inline   Pci *       list    { nullptr };        // PCI List

        static constexpr struct
        {
            uint32_t didvid;
            void (Pci::*func)();
        } quirks[] {};

        // Enhanced Configuration Space Address
        static constexpr auto ecam_addr (pci_t bdf, unsigned r = 0) { return MMAP_GLB_PCIE + bdf * PAGE_SIZE (0) + r; }

        // ECAM Read
        auto cfg_read (Register8  r) const { return *reinterpret_cast<uint8_t  volatile *>(ecam_addr (sid, std::to_underlying (r))); }
        auto cfg_read (Register16 r) const { return *reinterpret_cast<uint16_t volatile *>(ecam_addr (sid, std::to_underlying (r))); }
        auto cfg_read (Register32 r) const { return *reinterpret_cast<uint32_t volatile *>(ecam_addr (sid, std::to_underlying (r))); }

        // ECAM Write
        void cfg_write (Register8  r, uint8_t  v) const { *reinterpret_cast<uint8_t  volatile *>(ecam_addr (sid, std::to_underlying (r))) = v; }
        void cfg_write (Register16 r, uint16_t v) const { *reinterpret_cast<uint16_t volatile *>(ecam_addr (sid, std::to_underlying (r))) = v; }
        void cfg_write (Register32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(ecam_addr (sid, std::to_underlying (r))) = v; }

        static Pci *find_dev (pci_t s)
        {
            for (auto l { list }; l; l = l->next)
                if (l->sid == s)
                    return l;

            return nullptr;
        }
};
