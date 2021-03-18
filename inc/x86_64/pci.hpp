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

class Pci final
{
    friend class Acpi_table_mcfg;

    private:
        struct Pcap                     // PCI Capabilities
        {
            enum class Type : uint8_t
            {
                NULL        = 0x00,     // Null Capability
                PMI         = 0x01,     // Power Management Interface
                AGP         = 0x02,     // Accelerated Graphics Port
                VPD         = 0x03,     // Vital Product Data
                SLOT        = 0x04,     // Slot Identification
                MSI         = 0x05,     // Message Signaled Interrupts
                CP_HS       = 0x06,     // CompactPCI Hot Swap
                PCIX        = 0x07,     // PCI-X
                HT          = 0x08,     // HyperTransport
                VS          = 0x09,     // Vendor Specific
                DBGP        = 0x0a,     // Debug Port
                CP_RC       = 0x0b,     // CompactPCI Central Resource Control
                HOTPLUG     = 0x0c,     // PCI Hot-Plug
                SVID        = 0x0d,     // PCI Bridge Subsystem Vendor ID
                AGP8        = 0x0e,     // AGP 8x
                SDEV        = 0x0f,     // Secure Device
                PCIE        = 0x10,     // PCI Express
                MSIX        = 0x11,     // MSI-X
                SATA        = 0x12,     // Serial ATA Data/Index Configuration
                AF          = 0x13,     // Advanced Features
                EA          = 0x14,     // Enhanced Allocation
                FPB         = 0x15,     // Flattening Portal Bridge
            };

            uint8_t ptr { 0 };
        };

        struct Ecap                     // Extended Capabilities
        {
            enum class Type : uint16_t
            {
                NULL        = 0x0000,   // Null Capability
                AER         = 0x0001,   // Advanced Error Reporting
                RCRB        = 0x000a,   // Root Complex Register Block
                VSEC        = 0x000b,   // Vendor-Specific Extended Capability
                ACS         = 0x000d,   // Access Control Services
                ARI         = 0x000e,   // Alternative Routing-ID Interpretation
                ATS         = 0x000f,   // Address Translation Services
                SRIOV       = 0x0010,   // Single Root I/O Virtualization
                MRIOV       = 0x0011,   // Multi-Root I/O Virtualization
                MCAST       = 0x0012,   // Multicast
                PRI         = 0x0013,   // Page Request Interface
                PASID       = 0x001b,   // Process Address Space ID
            };

            uint16_t ptr { 0 };
        };

        struct Cap_pmi : Pcap
        {
            enum class Reg32 : unsigned
            {
                PMC_CAPID   = 0x00,     // Power Management Capabilities + CapID
                PMCSR       = 0x04,     // Power Management Control/Status
            };
        };

        struct Cap_pcix : Pcap
        {
        };

        struct Cap_pcie : Pcap
        {
            enum class Reg32 : unsigned
            {
                PEC_CAPID   = 0x00,     // PCI Express Capabilities + CapID
                DCAP        = 0x04,     // Device Capabilities
                DSTS_DCTL   = 0x08,     // Device Status + Device Control
                LCAP        = 0x0c,     // Link Capabilities
                LSTS_LCTL   = 0x10,     // Link Status + Link Control
                SCAP        = 0x14,     // Slot Capabilities
                SSTS_SCTL   = 0x18,     // Slot Status + Slot Control
                RCAP_RCTL   = 0x1c,     // Root Capabilities + Root Control
                RSTS        = 0x20,     // Root Status
                DCAP2       = 0x24,     // Device Capabilities 2
                DSTS2_DCTL2 = 0x28,     // Device Status 2 + Device Control 2
                LCAP2       = 0x2c,     // Link Capabilities 2
                LSTS2_LCTL2 = 0x30,     // Link Status 2 + Link Control 2
                SCAP2       = 0x34,     // Slot Capabilities 2
                SSTS2_SCTL2 = 0x38,     // Slot Status 2 + Slot Control 2
            };
        };

        struct Cap_sriov : Ecap
        {
            enum class Reg32 : unsigned
            {
                HDR         = 0x00,     // Extended Capability Header
                CAP         = 0x04,     // Capabilities Register
            };

            enum class Reg16 : unsigned
            {
                CTL         = 0x08,     // Control Register
                STS         = 0x0a,     // Status Register
                VFI         = 0x0c,     // VFs: Initial
                VFT         = 0x0e,     // VFs: Total
                VFN         = 0x10,     // VFs: Number
                FDL         = 0x12,     // Function Dependency Link
                VFO         = 0x14,     // VF Offset
                VFS         = 0x16,     // VF Stride
                VFD         = 0x1a,     // VF Device ID
            };
        };

    public:
        struct Cfg final
        {
            enum class Reg32 : unsigned
            {
                DID_VID     = 0x00,     // Device ID + Vendor ID
                CCP_RID     = 0x08,     // Class Codes + Programming Interface + Revision ID
                BAR_0       = 0x10,     // Base Address Register 0
                BAR_1       = 0x14,     // Base Address Register 1
                BUS_NUM     = 0x18,     // Bus Numbers
            };

            enum class Reg16 : unsigned
            {
                CMD         = 0x04,     // Command Register
                STS         = 0x06,     // Status Register
            };

            enum class Reg8 : unsigned
            {
                HDR         = 0x0e,     // Header Type
                CAP         = 0x34,     // Capabilities Pointer
            };
        };

        class Device final : public List<Device>, private Cap_pmi, private Cap_pcix, private Cap_pcie, private Cap_sriov
        {
            private:
                pci_t   const   sid;
                uint8_t const   lev;
                Smmu *          smmu { nullptr };

                static          Slab_cache  cache;                      // Device Slab Cache
                static inline   Device *    list    { nullptr };        // Device List

                void enumerate_pcap();
                void enumerate_ecap();

                static Device *find_dev (pci_t s)
                {
                    for (auto l { list }; l; l = l->next)
                        if (l->sid == s)
                            return l;

                    return nullptr;
                }

            public:
                Device (pci_t, uint8_t);

                // Configuration Read
                auto read (Cfg::Reg8  r) const { return *reinterpret_cast<uint8_t  volatile *>(ecam_addr (sid, std::to_underlying (r))); }
                auto read (Cfg::Reg16 r) const { return *reinterpret_cast<uint16_t volatile *>(ecam_addr (sid, std::to_underlying (r))); }
                auto read (Cfg::Reg32 r) const { return *reinterpret_cast<uint32_t volatile *>(ecam_addr (sid, std::to_underlying (r))); }

                // Configuration Write
                void write (Cfg::Reg8  r, uint8_t  v) const { *reinterpret_cast<uint8_t  volatile *>(ecam_addr (sid, std::to_underlying (r))) = v; }
                void write (Cfg::Reg16 r, uint16_t v) const { *reinterpret_cast<uint16_t volatile *>(ecam_addr (sid, std::to_underlying (r))) = v; }
                void write (Cfg::Reg32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(ecam_addr (sid, std::to_underlying (r))) = v; }

                // Capability Pointer
                template <typename T> auto cap() const { return static_cast<T const *>(this)->ptr; }

                // Capability Read
                template <typename T> auto read (T::Reg8  r) const { return read (Cfg::Reg8  { cap<T>() + std::to_underlying (r) }); }
                template <typename T> auto read (T::Reg16 r) const { return read (Cfg::Reg16 { cap<T>() + std::to_underlying (r) }); }
                template <typename T> auto read (T::Reg32 r) const { return read (Cfg::Reg32 { cap<T>() + std::to_underlying (r) }); }

                // Capability Write
                template <typename T> void write (T::Reg8  r, uint8_t  v) const { write (Cfg::Reg8  { cap<T>() + std::to_underlying (r) }, v); };
                template <typename T> void write (T::Reg16 r, uint16_t v) const { write (Cfg::Reg16 { cap<T>() + std::to_underlying (r) }, v); };
                template <typename T> void write (T::Reg32 r, uint32_t v) const { write (Cfg::Reg32 { cap<T>() + std::to_underlying (r) }, v); };

                static void claim_all (Smmu *s)
                {
                    for (auto l { list }; l; l = l->next)
                        if (!l->smmu)
                            l->smmu = s;
                }

                static bool claim_dev (Smmu *s, pci_t d)
                {
                    auto dev { find_dev (d) };

                    if (!dev)
                        return false;

                    auto const l { dev->lev };

                    do dev->smmu = s; while ((dev = dev->next) && dev->lev > l);

                    return true;
                }

                static Smmu *find_smmu (pci_t d)
                {
                    auto dev { find_dev (d) };

                    return dev ? dev->smmu : nullptr;
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
        };

    private:
        // Enhanced Configuration Space Address
        static constexpr uintptr_t ecam_addr (pci_t bdf, unsigned r = 0) { return MMAP_GLB_PCIE + bdf * PAGE_SIZE (0) + r; }

        // Configuration Space Address
        static void addr (pci_t bdf, unsigned r) { Io::out<uint32_t>(0xcf8, BIT (31) | bdf << 8 | r); }

        // Configuration Space Data
        static constexpr auto data (Cfg::Reg8  r) { return static_cast<port_t>(0xcfc | (std::to_underlying (r) & 3)); }
        static constexpr auto data (Cfg::Reg16 r) { return static_cast<port_t>(0xcfc | (std::to_underlying (r) & 2)); }
        static constexpr auto data (Cfg::Reg32)   { return static_cast<port_t>(0xcfc); }

    public:
        // Configuration Space Read
        static auto read (pci_t bdf, Cfg::Reg8  r) { addr (bdf, std::to_underlying (r)); return Io::in<uint8_t> (data (r)); }
        static auto read (pci_t bdf, Cfg::Reg16 r) { addr (bdf, std::to_underlying (r)); return Io::in<uint16_t>(data (r)); }
        static auto read (pci_t bdf, Cfg::Reg32 r) { addr (bdf, std::to_underlying (r)); return Io::in<uint32_t>(data (r)); }

        // Configuration Space Write
        static void write (pci_t bdf, Cfg::Reg8  r, uint8_t  v) { addr (bdf, std::to_underlying (r)); Io::out<uint8_t> (data (r), v); }
        static void write (pci_t bdf, Cfg::Reg16 r, uint16_t v) { addr (bdf, std::to_underlying (r)); Io::out<uint16_t>(data (r), v); }
        static void write (pci_t bdf, Cfg::Reg32 r, uint32_t v) { addr (bdf, std::to_underlying (r)); Io::out<uint32_t>(data (r), v); }

        // Configuration Space Read/Write
        static void rd_wr (pci_t bdf, Cfg::Reg8  r, uint8_t  s) { addr (bdf, std::to_underlying (r)); Io::out<uint8_t> (data (r), Io::in<uint8_t> (data (r)) | s); }
        static void rd_wr (pci_t bdf, Cfg::Reg16 r, uint16_t s) { addr (bdf, std::to_underlying (r)); Io::out<uint16_t>(data (r), Io::in<uint16_t>(data (r)) | s); }
        static void rd_wr (pci_t bdf, Cfg::Reg32 r, uint32_t s) { addr (bdf, std::to_underlying (r)); Io::out<uint32_t>(data (r), Io::in<uint32_t>(data (r)) | s); }

        // Compose B:D:F triplet
        static constexpr auto bdf (uint8_t b, uint8_t d, uint8_t f) { return static_cast<pci_t>(b << 8 | d << 3 | f); }

        // Decompose B:D:F triplet
        static constexpr auto bus (pci_t bdf) { return static_cast<uint8_t>(bdf >> 8); }
        static constexpr auto dev (pci_t bdf) { return static_cast<uint8_t>(BIT_RANGE (4, 0) & bdf >> 3); }
        static constexpr auto fun (pci_t bdf) { return static_cast<uint8_t>(BIT_RANGE (2, 0) & bdf); }

        static constexpr struct
        {
            uint32_t didvid;
            void (Device::*func)();
        } quirks[] {};

        static uint8_t init (uint8_t, uint8_t, uint8_t = 0);

        static bool match_list (uint16_t id, uint16_t const list[])
        {
            for (auto ptr { list }; *ptr; ptr++)
                if (*ptr == id)
                    return true;

            return false;
        }
};
