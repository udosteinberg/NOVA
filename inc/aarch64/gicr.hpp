/*
 * Generic Interrupt Controller: Redistributor (GICR)
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "bits.hpp"
#include "buddy.hpp"
#include "coherency.hpp"
#include "coresight.hpp"
#include "intid.hpp"
#include "status.hpp"
#include "wait.hpp"

class Gicr final : private Coresight, private Intid
{
    friend class Acpi_table_madt;

    private:
        enum class Reg32 : unsigned     // RD_BASE
        {
            CTLR            = 0x00000,  // -- v3 rw Control Register
            IIDR            = 0x00004,  // -- v3 r- Implementer Identification Register
            STATUSR         = 0x00010,  // -- v3 rw Error Reporting Status Register
            WAKER           = 0x00014,  // -- v3 rw Wake Register
            MPAMIDR         = 0x00018,  // -- v3 r- Max PARTID and PMG Register
            PARTIDR         = 0x0001c,  // -- v3 rw Set PARTID and PMG Register
            SYNCR           = 0x000c0,  // -- v3 r- Synchronize Register
        };

        enum class Reg64 : unsigned     // RD_BASE
        {
            TYPER           = 0x00008,  // -- v3 r- Type Register
            SETLPIR         = 0x00040,  // -- v3 -w Set LPI Pending Register
            CLRLPIR         = 0x00048,  // -- v3 -w Clr LPI Pending Register
            PROPBASER       = 0x00070,  // -- v3 rw Properties Base Address Register
            PENDBASER       = 0x00078,  // -- v3 rw LPI Pending Table Base Address Register
            INVLPIR         = 0x000a0,  // -- v3 -w Invalidate LPI Register
            INVALLR         = 0x000b0,  // -- v3 -w Invalidate All Register
        };

        enum class Arr32 : unsigned     // SGI_BASE
        {
            IGROUPR         = 0x10080,  // -- v3 rw Standard Interrupt Group Registers
            ISENABLER       = 0x10100,  // -- v3 rw Standard Interrupt Set-Enable Registers
            ICENABLER       = 0x10180,  // -- v3 rw Standard Interrupt Clr-Enable Registers
            ISPENDR         = 0x10200,  // -- v3 rw Standard Interrupt Set-Pending Registers
            ICPENDR         = 0x10280,  // -- v3 rw Standard Interrupt Clr-Pending Registers
            ISACTIVER       = 0x10300,  // -- v3 rw Standard Interrupt Set-Active Registers
            ICACTIVER       = 0x10380,  // -- v3 rw Standard Interrupt Clr-Active Registers
            IPRIORITYR      = 0x10400,  // -- v3 rw Standard Interrupt Priority Registers
            ICFGR           = 0x10c00,  // -- v3 rw Standard SGI/PPI Configuration Registers
            IGRPMODER       = 0x10d00,  // -- v3 rw Standard Interrupt Group Modifier Registers
            INMIR           = 0x10f80,  // -- v3 rw Standard Non-Maskable Interrupt Registers
        };

        static auto read  (Reg32 r)                  { return *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_GICR + std::to_underlying (r)); }
        static auto read  (Reg64 r)                  { return *reinterpret_cast<uint64_t volatile *>(MMAP_CPU_GICR + std::to_underlying (r)); }
        static auto read  (Arr32 r, unsigned n)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_GICR + std::to_underlying (r) + n * sizeof (uint32_t)); }

        static void write (Reg32 r,             uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_GICR + std::to_underlying (r)) = v; }
        static void write (Reg64 r,             uint64_t v) { *reinterpret_cast<uint64_t volatile *>(MMAP_CPU_GICR + std::to_underlying (r)) = v; }
        static void write (Arr32 r, unsigned n, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_GICR + std::to_underlying (r) + n * sizeof (uint32_t)) = v; }

        /*
         * LPI Configuration Table (5.1.1)
         */
        class Cfg_table
        {
            private:
                Atomic<uint8_t> val;

            public:
                void set (bool msk, bool c)
                {
                    // Apply configuration
                    val = BIT (1) | !msk * BIT (0);

                    // Make update observable
                    Coherency::observe (&val, sizeof (val), c);
                }

                [[nodiscard]] static void *operator new (size_t, unsigned o, bool c) noexcept
                {
                    // Order of entries that fit into a page
                    static constexpr unsigned p { bit_scan_msb (PAGE_SIZE (0) / sizeof (val)) };

                    // Allocate storage for 2^o entries (but at least 4K-aligned)
                    auto const ord { static_cast<uint8_t>(max (0U, max (o, p) - p)) };
                    auto const ptr { Buddy::alloc (ord, Buddy::Fill::BITS0) };

                    // Make table observable
                    if (ptr) [[likely]]
                        Coherency::observe (ptr, PAGE_SIZE (0) << ord, c);

                    return ptr;
                }
        };

        /*
         * LPI Pending Table (5.1.2)
         */
        class Pnd_table
        {
            public:
                [[nodiscard]] static void *operator new (size_t, unsigned o, bool c) noexcept
                {
                    // Order of entries that fit into a page
                    static constexpr unsigned p { bit_scan_msb (PAGE_SIZE (0) * 8) };

                    // Allocate storage for 2^o entries (but at least 64K-aligned)
                    auto const ord { static_cast<uint8_t>(max (4U, max (o, p) - p)) };
                    auto const ptr { Buddy::alloc (ord, Buddy::Fill::BITS0) };

                    // Make table observable
                    if (ptr) [[likely]]
                        Coherency::observe (ptr, PAGE_SIZE (0) << ord, c);

                    return ptr;
                }
        };

        static inline constinit Cfg_table * cfg_table           { nullptr };
        static inline constinit Pnd_table * pnd_table CPULOCAL  { nullptr };
        static inline constinit bool        coherent            { true };

        static constexpr auto mmio_size { 0x20000 };

        static constexpr auto impl_bits { VAL64_SHIFT (3, 10) | VAL64_SHIFT (7, 56) };  // Implementation-Defined Fixed-Value Bits
        static constexpr auto attr_isic { VAL64_SHIFT (1, 10) | VAL64_SHIFT (7,  7) };  // Inner Shareable + Inner Cacheable (RA/WA/WB)
        static constexpr auto attr_nsnc { VAL64_SHIFT (0, 10) | VAL64_SHIFT (1,  7) };  // Non-Shareable + Non-Cacheable

        [[nodiscard]] static bool mmap_mmio();
        [[nodiscard]] static bool init_mmio();

        [[nodiscard]] static bool wait_rwp()
        {
            return Wait::until (1, [&] { return (read (Reg32::CTLR) & BIT (3)) == 0; });
        }

        [[nodiscard]] static bool set_ctlr (uint32_t v)
        {
            write (Reg32::CTLR, v);

            return wait_rwp();
        }

        [[nodiscard]] static bool set_sleep (bool b)
        {
            // Update ProcessorSleep
            write (Reg32::WAKER, BIT (1) * b);

            // Poll ChildrenAsleep
            return Wait::until (1, [&] { return (read (Reg32::WAKER) & BIT (2)) == BIT (2) * b; });
        }

        static inline void init_intid (arm_intid_t, unsigned);

    public:
        [[nodiscard]] static bool enumerate (uint64_t, uint32_t);

        static void init();

        static bool act_get (arm_intid_t);
        static void act_set (arm_intid_t, bool);

        static Status conf_ppi (arm_intid_t, bool = false, bool = false);
        static Status conf_lpi (arm_intid_t, cpu_t, pci_t, arm_evtid_t, bool, uintptr_t &, uintptr_t &);
};
