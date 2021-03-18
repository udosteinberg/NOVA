/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

        // Compose BDF triplet
        static constexpr auto bdf (unsigned b, unsigned d, unsigned f) { return static_cast<uint16>(b << 8 | d << 3 | f); }

        // Configuration Space Address
        static void cfg_addr (uint16 bdf, unsigned r) { Io::out<uint32>(0xcf8, BIT (31) | bdf << 8 | r); }

        // Configuration Space Read
        static auto cfg_read (uint16 bdf, Register8  r) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc | (std::to_underlying (r) & 3) }; return Io::in<uint8> (p); }
        static auto cfg_read (uint16 bdf, Register16 r) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc | (std::to_underlying (r) & 2) }; return Io::in<uint16>(p); }
        static auto cfg_read (uint16 bdf, Register32 r) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc };                                return Io::in<uint32>(p); }

        // Configuration Space Write
        static void cfg_write (uint16 bdf, Register8  r, uint8  v) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc | (std::to_underlying (r) & 3) }; Io::out<uint8> (p, v); }
        static void cfg_write (uint16 bdf, Register16 r, uint16 v) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc | (std::to_underlying (r) & 2) }; Io::out<uint16>(p, v); }
        static void cfg_write (uint16 bdf, Register32 r, uint32 v) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc };                                Io::out<uint32>(p, v); }

        // Configuration Space Read/Write
        static void cfg_rd_wr (uint16 bdf, Register8  r, uint8  s) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc | (std::to_underlying (r) & 3) }; Io::out<uint8> (p, Io::in<uint8> (p) | s); }
        static void cfg_rd_wr (uint16 bdf, Register16 r, uint16 s) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc | (std::to_underlying (r) & 2) }; Io::out<uint16>(p, Io::in<uint16>(p) | s); }
        static void cfg_rd_wr (uint16 bdf, Register32 r, uint32 s) { cfg_addr (bdf, std::to_underlying (r)); auto p { 0xcfc };                                Io::out<uint32>(p, Io::in<uint32>(p) | s); }

        Pci (unsigned, unsigned);

        static void init (unsigned = 0, unsigned = 0);

        ALWAYS_INLINE
        static inline void claim_all (Smmu *s)
        {
            for (auto pci { list }; pci; pci = pci->next)
                if (!pci->smmu)
                    pci->smmu = s;
        }

        ALWAYS_INLINE
        static inline bool claim_dev (Smmu *s, unsigned r)
        {
            auto pci { find_dev (r) };

            if (!pci)
                return false;

            unsigned const l { pci->lev };

            do pci->smmu = s; while ((pci = pci->next) && pci->lev > l);

            return true;
        }

        ALWAYS_INLINE
        static inline Smmu *find_smmu (unsigned long r)
        {
            auto const pci { find_dev (r) };

            return pci ? pci->smmu : nullptr;
        }

        [[nodiscard]] ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    private:
        uintptr_t   const   reg_base;
        uint16      const   rid;
        uint16      const   lev;
        Smmu *              smmu;

        static          Slab_cache  cache;                      // PCI Slab Cache
        static inline   Pci *       list    { nullptr };        // PCI List
        static inline   uintptr_t   mmap    { MMAP_GLB_PCIE };  // PCI Memory Map Pointer
        static inline   unsigned    bus_base;
        static inline   Paddr       cfg_base;
        static inline   size_t      cfg_size;

        static struct quirk_map
        {
            uint32 didvid;
            void (Pci::*func)();
        } map[];

        // ECAM Read
        auto cfg_read (Register8  r) const { return *reinterpret_cast<uint8  volatile *>(reg_base + std::to_underlying (r)); }
        auto cfg_read (Register16 r) const { return *reinterpret_cast<uint16 volatile *>(reg_base + std::to_underlying (r)); }
        auto cfg_read (Register32 r) const { return *reinterpret_cast<uint32 volatile *>(reg_base + std::to_underlying (r)); }

        // ECAM Write
        void cfg_write (Register8  r, uint8  v) const { *reinterpret_cast<uint8  volatile *>(reg_base + std::to_underlying (r)) = v; }
        void cfg_write (Register16 r, uint16 v) const { *reinterpret_cast<uint16 volatile *>(reg_base + std::to_underlying (r)) = v; }
        void cfg_write (Register32 r, uint32 v) const { *reinterpret_cast<uint32 volatile *>(reg_base + std::to_underlying (r)) = v; }

        ALWAYS_INLINE
        static inline Pci *find_dev (unsigned long r)
        {
            for (auto pci { list }; pci; pci = pci->next)
                if (pci->rid == r)
                    return pci;

            return nullptr;
        }
};
