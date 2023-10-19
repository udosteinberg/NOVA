/*
 * PCI Configuration Space
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

#include "list.hpp"
#include "memory.hpp"
#include "slab.hpp"
#include "std.hpp"

class Smmu;

class Pci final
{
    private:
        /*
         * PCI Code and ID Assignment Specification 1.19, Section 2: Capability IDs
         */
        struct Pcap
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

            uint8_t off {};             // Capability Offset (0 if not present)
        };

        /*
         * PCI Code and ID Assignment Specification 1.19, Section 3: Extended Capability IDs
         */
        struct Ecap
        {
            enum class Type : uint16_t
            {
                NULL        = 0x0000,   // Null Capability
                AER         = 0x0001,   // Advanced Error Reporting
                VC          = 0x0002,   // Virtual Channel
                DSN         = 0x0003,   // Device Serial Number
                PWR         = 0x0004,   // Power Budgeting
                RCLD        = 0x0005,   // Root Complex Link Declaration
                RCLC        = 0x0006,   // Root Complex Link Control
                RCEC        = 0x0007,   // Root Complex Event Collector
                MFVC        = 0x0008,   // Multi-Function Virtual Channel
                VCEX        = 0x0009,   // Virtual Channel
                RCRB        = 0x000a,   // Root Complex Register Block
                VSEC        = 0x000b,   // Vendor-Specific Extended Capability
                CAC         = 0x000c,   // Configuration Access Correlation
                ACS         = 0x000d,   // Access Control Services
                ARI         = 0x000e,   // Alternative Routing-ID Interpretation
                ATS         = 0x000f,   // Address Translation Services
                SRIOV       = 0x0010,   // Single Root I/O Virtualization
                MRIOV       = 0x0011,   // Multi-Root I/O Virtualization
                MCAST       = 0x0012,   // Multicast
                PRI         = 0x0013,   // Page Request Interface
                RBAR        = 0x0015,   // Resizable Bar
                DPA         = 0x0016,   // Dynamic Power Allocation
                PASID       = 0x001b,   // Process Address Space ID
            };

            uint16_t off {};            // Capability Offset (0 if not present)
        };

        static constexpr auto cfg_size { 4096 };
        static constexpr auto seg_shft {   16 };
        static constexpr auto bus_shft {    8 };
        static constexpr auto dev_shft {    3 };

        // Number of mappable PCI Segment Groups
        static constexpr auto seg_grps { (MMAP_GLB_PCIE - MMAP_GLB_PCIS) / (cfg_size << seg_shft) };

        // Enhanced Configuration Space Address
        static constexpr uintptr_t ecam_addr (pci_t sbdf, unsigned r = 0) { return MMAP_GLB_PCIS + sbdf * cfg_size + r; }

        static uint8_t init_bus (uint16_t, uint8_t, uint8_t, uint8_t);

    public:
        static constexpr auto pci (            uint8_t b, uint8_t d, uint8_t f) { return static_cast<pci_t>(                b << bus_shft | d << dev_shft | f); }
        static constexpr auto pci (uint16_t s, uint8_t b, uint8_t d, uint8_t f) { return static_cast<pci_t>(s << seg_shft | b << bus_shft | d << dev_shft | f); }
        static constexpr auto pci (uint16_t s, uint16_t bdf)                    { return static_cast<pci_t>(s << seg_shft | bdf); }

        static constexpr auto seg (pci_t p) { return static_cast<uint16_t>(p >> seg_shft); }
        static constexpr auto bdf (pci_t p) { return static_cast<uint16_t>(p); }
        static constexpr auto bus (pci_t p) { return static_cast<uint8_t> (p >> bus_shft); }
        static constexpr auto ari (pci_t p) { return static_cast<uint8_t> (p); }
        static constexpr auto dev (pci_t p) { return static_cast<uint8_t> (p >> dev_shft & BIT_RANGE (4, 0)); }
        static constexpr auto fun (pci_t p) { return static_cast<uint8_t> (p             & BIT_RANGE (2, 0)); }

        [[nodiscard]] static bool init_seg (uint64_t, uint16_t, uint8_t, uint8_t);

        /*
         * PCI Express Base Specification 7.0, Section 7.5.1: PCI-Compatible Configuration Registers
         */
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

        /*
         * PCI Express Base Specification 7.0, Section 7.5.2: PCI Power Management Capability Structure
         */
        struct Cap_pmi : Pcap
        {
            // Use Argument-Dependent Lookup to link Cap_pmi::RegN to Cap_pmi
            template<typename X> friend Cap_pmi enclosing_class (X);

            enum class Reg32 : unsigned
            {
                HDR         = 0x00,     // Capability Header
                PMCSR       = 0x04,     // Power Management Control/Status
            };
        };

        /*
         * PCI Express Base Specification 7.0, Section 7.5.3: PCI Express Capability Structure
         */
        struct Cap_pcie : Pcap
        {
            // Use Argument-Dependent Lookup to link Cap_pcie::RegN to Cap_pcie
            template<typename X> friend Cap_pcie enclosing_class (X);

            enum class Reg32 : unsigned
            {
                HDR         = 0x00,     // Capability Header
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

        /*
         * PCI Express Base Specification 7.0, Section 7.7.1: MSI Capability Structure
         */
        struct Cap_msi : Pcap
        {
            // Use Argument-Dependent Lookup to link Cap_msi::RegN to Cap_msi
            template<typename X> friend Cap_msi enclosing_class (X);

            enum class Reg32 : unsigned
            {
                HDR         = 0x00,     // Capability Header + Message Control
                F32_ADDR    = 0x04,     // 32-bit Format: Message Addr
                F32_DATA    = 0x08,     // 32-bit Format: Message Data
                F64_ADDR_LO = 0x04,     // 64-bit Format: Message Addr Lo
                F64_ADDR_HI = 0x08,     // 64-bit Format: Message Addr Hi
                F64_DATA    = 0x0c,     // 64-bit Format: Message Data
            };
        };

        /*
         * PCI Express Base Specification 7.0, Section 7.7.2: MSI-X Capability Structure
         */
        struct Cap_msix : Pcap
        {
        };

        /*
         * AMD I/O Virtualization Technology (IOMMU) Specification, Section 3.2
         */
        struct Cap_sdev : Pcap
        {
            // Use Argument-Dependent Lookup to link Cap_sdev::RegN to Cap_sdev
            template<typename X> friend Cap_sdev enclosing_class (X);

            enum class Reg32 : unsigned
            {
                HDR         = 0x00,     // Capability Header
                BAR_LO      = 0x04,     // Base Address Lo
                BAR_HI      = 0x08,     // Base Address Hi
                RANGE       = 0x0c,     // Range
                MISC_0      = 0x10,     // Miscellaneous Information 0
                MISC_1      = 0x14,     // Miscellaneous Information 1
            };
        };

        /*
         * PCI Express Base Specification 7.0, Section 9.4.3: SR-IOV Extended Capability
         */
        struct Cap_sriov : Ecap
        {
            // Use Argument-Dependent Lookup to link Cap_sriov::RegN to Cap_sriov
            template<typename X> friend Cap_sriov enclosing_class (X);

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

        /*
         * PCI Express Base Specification 7.0, Section 10.5.1: ATS Extended Capability
         */
        struct Cap_ats : Ecap
        {
            // Use Argument-Dependent Lookup to link Cap_ats::RegN to Cap_ats
            template<typename X> friend Cap_ats enclosing_class (X);

            enum class Reg32 : unsigned
            {
                HDR         = 0x00,     // Extended Capability Header
                CAP_CTL     = 0x04,     // Capability + Control Register
            };
        };

        /*
         * PCI Express Base Specification 7.0, Section 10.5.2: Page Request Extended Capability
         */
        struct Cap_pri : Ecap
        {
            // Use Argument-Dependent Lookup to link Cap_pri::RegN to Cap_pri
            template<typename X> friend Cap_pri enclosing_class (X);

            enum class Reg32 : unsigned
            {
                HDR         = 0x00,     // Extended Capability Header
                CTL_STS     = 0x04,     // Control + Status Register
            };
        };

        class Function final : public List<Function>, private Cap_pmi, private Cap_pcie, private Cap_msi, private Cap_msix, private Cap_sdev, private Cap_sriov, private Cap_ats, private Cap_pri
        {
            private:
                pci_t   const   sbdf;
                uint8_t const   lev;
                Smmu *          smmu { nullptr };

                static Slab_cache cache;                                // Function Slab Cache
                static inline constinit Function *list { nullptr };     // Function List

                void enumerate_pcap();
                void enumerate_ecap();

            public:
                Function (pci_t, uint8_t);

                // Configuration Read   (always 1 DWORD == 32-bit with byte enables)
                auto read (Cfg::Reg8  r) const { return *std::start_lifetime_as<uint8_t  volatile> (ecam_addr (sbdf, std::to_underlying (r))); }
                auto read (Cfg::Reg16 r) const { return *std::start_lifetime_as<uint16_t volatile> (ecam_addr (sbdf, std::to_underlying (r))); }
                auto read (Cfg::Reg32 r) const { return *std::start_lifetime_as<uint32_t volatile> (ecam_addr (sbdf, std::to_underlying (r))); }

                // Configuration Write  (always 1 DWORD == 32-bit with byte enables)
                void write (Cfg::Reg8  r, uint8_t  v) const { *std::start_lifetime_as<uint8_t  volatile> (ecam_addr (sbdf, std::to_underlying (r))) = v; }
                void write (Cfg::Reg16 r, uint16_t v) const { *std::start_lifetime_as<uint16_t volatile> (ecam_addr (sbdf, std::to_underlying (r))) = v; }
                void write (Cfg::Reg32 r, uint32_t v) const { *std::start_lifetime_as<uint32_t volatile> (ecam_addr (sbdf, std::to_underlying (r))) = v; }

                // Capability C Offset
                template<typename C> auto cap() const { return static_cast<C const *>(this)->off; }

                // Capability Register R Read
                template<typename R> auto read (R r) const requires std::same_as<R, typename decltype (enclosing_class (r))::Reg8>  { return read (Cfg::Reg8  { cap<decltype (enclosing_class (r))>() + std::to_underlying (r) }); }
                template<typename R> auto read (R r) const requires std::same_as<R, typename decltype (enclosing_class (r))::Reg16> { return read (Cfg::Reg16 { cap<decltype (enclosing_class (r))>() + std::to_underlying (r) }); }
                template<typename R> auto read (R r) const requires std::same_as<R, typename decltype (enclosing_class (r))::Reg32> { return read (Cfg::Reg32 { cap<decltype (enclosing_class (r))>() + std::to_underlying (r) }); }

                // Capability Register R Write
                template<typename R> void write (R r, uint8_t  v) const requires std::same_as<R, typename decltype (enclosing_class (r))::Reg8>  { write (Cfg::Reg8  { cap<decltype (enclosing_class (r))>() + std::to_underlying (r) }, v); }
                template<typename R> void write (R r, uint16_t v) const requires std::same_as<R, typename decltype (enclosing_class (r))::Reg16> { write (Cfg::Reg16 { cap<decltype (enclosing_class (r))>() + std::to_underlying (r) }, v); }
                template<typename R> void write (R r, uint32_t v) const requires std::same_as<R, typename decltype (enclosing_class (r))::Reg32> { write (Cfg::Reg32 { cap<decltype (enclosing_class (r))>() + std::to_underlying (r) }, v); }

                [[nodiscard]] bool configure_msi (uint64_t addr, uint32_t data, unsigned o = 0)
                {
                    // Check for MSI capability
                    if (!cap<Cap_msi>()) [[unlikely]]
                        return false;

                    auto h { read (Cap_msi::Reg32::HDR) };

                    // MSI order must not exceed supported order
                    if (o > (h >> 17 & BIT_RANGE (2, 0))) [[unlikely]]
                        return false;

                    // MSI data must be order-aligned
                    if (data & (BIT (o) - 1)) [[unlikely]]
                        return false;

                    // Configure MSI
                    if (h & BIT (23)) [[likely]] {      // 64-bit
                        write (Cap_msi::Reg32::F64_ADDR_LO, static_cast<uint32_t>(addr));
                        write (Cap_msi::Reg32::F64_ADDR_HI, static_cast<uint32_t>(addr >> 32));
                        write (Cap_msi::Reg32::F64_DATA, data);
                    } else {                            // 32-bit
                        write (Cap_msi::Reg32::F32_ADDR, static_cast<uint32_t>(addr));
                        write (Cap_msi::Reg32::F32_DATA, data);
                    }

                    // Enable MSI
                    write (Cap_msi::Reg32::HDR, h = o << 20 | BIT (16));

                    // Check all writable bits
                    return (read (Cap_msi::Reg32::HDR) & (BIT (26) | BIT_RANGE (22, 20) | BIT (16))) == h;
                }

                static Function *lookup (pci_t sbdf)
                {
                    for (auto fun { list }; fun; fun = fun->next)
                        if (fun->sbdf == sbdf)
                            return fun;

                    return nullptr;
                }

                static void claim_all (Smmu *s)
                {
                    for (auto l { list }; l; l = l->next)
                        if (!l->smmu)
                            l->smmu = s;
                }

                static bool claim_dev (Smmu *s, pci_t sbdf)
                {
                    auto dev { lookup (sbdf) };

                    if (!dev)
                        return false;

                    auto const l { dev->lev };

                    do dev->smmu = s; while ((dev = dev->next) && dev->lev > l);

                    return true;
                }

                static Smmu *find_smmu (pci_t sbdf)
                {
                    auto const dev { lookup (sbdf) };

                    return dev ? dev->smmu : nullptr;
                }

                [[nodiscard]] static void *operator new (size_t) noexcept
                {
                    return cache.alloc();
                }

                static void operator delete (void *ptr)
                {
                    cache.free (ptr);
                }
        };
};
