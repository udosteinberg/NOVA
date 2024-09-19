/*
 * System Memory Management Unit (Abstraction Layer)
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

#pragma once

#include "barrier.hpp"
#include "cmdline.hpp"
#include "coherence.hpp"
#include "hip.hpp"
#include "intid.hpp"
#include "lock_guard.hpp"
#include "mmio.hpp"
#include "pci.hpp"
#include "status.hpp"
#include "wait.hpp"

class Pd;

class Smmu : public List<Smmu>, protected Mmio
{
    public:
        static inline constinit bool noir   { false };
        static inline constinit bool x2apic { true };

        enum class Type { NONE, AMD, ITL };

        [[nodiscard]] static auto type() { return hwtype; }

        // Table Entry
        class Entry
        {
            template<typename, int, int, int> friend class Atomic;

            protected:
                mutable uint128_t val;

                explicit constexpr Entry (uint128_t v) : val { v } {}
        };

        static_assert (__is_standard_layout (Entry) && alignof (Entry) == 16 && sizeof (Entry) == 16);

        virtual Status assign_dev (uintptr_t, Pd *, Pd *, uintptr_t &) = 0;

        // Interrupt Source Encoding for MSI: [31]=valid, [30:16]=idx, [15:0]=bdf
        static uint32_t ise_msi (pci_t src, uint16_t idx) { return BIT (31) | uint32_t { idx } << 16 | Pci::bdf (src); }

        [[nodiscard]] virtual Status irte_get (Atomic<Entry> *&, Intid, pci_t, uint16_t, bool) = 0;
        [[nodiscard]] virtual Status irte_set (Atomic<Entry> *, Atomic<uintptr_t> &, bool, Intid, pci_t, apic_t, uint8_t, bool, uint16_t) = 0;
        [[nodiscard]] virtual Status irte_clr (Atomic<Entry> *, Atomic<uintptr_t> &, bool, Intid, pci_t, apic_t, uint8_t) = 0;

        static void all_interrupt()
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                smmu->interrupt();
        }

        /*
         * Invoke invalidation function for all SMMUs in the PCI segment group
         */
        template<typename T> [[nodiscard]] static bool seg_invalidate (uint16_t seg, auto const &func)
        {
            bool ret { true };

            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (Pci::seg (smmu->sbdf) == seg)
                    ret &= func (static_cast<T *>(smmu));

            return ret;
        }

        /*
         * Invalidate TLB for all SMMUs
         */
        [[nodiscard]] static bool all_invalidate_tlb (uint16_t dom)
        {
            bool ret { true };

            for (auto smmu { list }; smmu; smmu = smmu->next)
                ret &= smmu->invalidate_tlb (dom);

            return ret;
        }

        /*
         * Lookup SMMU based on physical address
         */
        [[nodiscard]] static Smmu *lookup_phys (uint64_t phys)
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (smmu->phys == phys)
                    return smmu;

            return nullptr;
        }

        /*
         * Lookup SMMU based on PCI segment group
         */
        [[nodiscard]] static Smmu *lookup_seg (uint16_t seg)
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (Pci::seg (smmu->sbdf) == seg)
                    return smmu;

            return nullptr;
        }

        /*
         * Initialize all SMMUs
         */
        [[nodiscard]] static bool initialize()
        {
            bool ret { true };

            if (!Cmdline::nosmmu) [[likely]] {

                for (auto smmu { list }; smmu; smmu = smmu->next)
                    ret &= smmu->init();

                if (list && ret) [[likely]]
                    Hip::hip->set_feature (Hip::FEAT_IOMMU);
            }

            return ret;
        }

    protected:
        // Timeout for SMMU operations (in ms)
        static constexpr unsigned timeout { 5 };

        // PCI Seg:Bus:Dev:Fun
        pci_t const sbdf;

        // List of SMMUs
        static inline constinit Smmu *list {};

        // Set SMMU list type
        [[nodiscard]] static bool set_type (Type t)
        {
            if (hwtype == Type::NONE)
                hwtype = t;

            return hwtype == t;
        }

        /*
         * Generic Circular Queue
         */
        class Queue
        {
            private:
                using index_t = uint32_t;

                index_t slot (index_t i) const { return i & (BIT (ord) - 1); }

            public:
                class alignas (16) Entry
                {
                    protected:
                        uint64_t lo, hi;

                    public:
                        explicit constexpr Entry() = default;

                        explicit constexpr Entry (uint64_t l, uint64_t h) : lo { l }, hi { h } {}

                        [[nodiscard]] ALWAYS_INLINE static void *operator new[] (size_t s) noexcept { return Buddy::allocator.alloc (static_cast<unsigned short>(max (PAGE_BITS, bit_scan_msb (s)) - PAGE_BITS)); }
                };

                static_assert (__is_standard_layout (Entry) && alignof (Entry) == 16 && sizeof (Entry) == 16);

                unsigned  const ord;    // Order of Entries
                Entry *   const ptr;    // Base Pointer
                index_t         swi;    // Software Index
                Spinlock        lock;   // Queue Lock

                // Return physical base address of queue
                [[nodiscard]] auto base() const { return Kmem::ptr_to_phys (ptr); }

                // Return software offset from software index
                [[nodiscard]] size_t offs() const { return slot (swi) * sizeof (Entry); }

                // Return hardware index from hardware offset (sanitized)
                [[nodiscard]] auto hwi (unsigned offs) const { return slot (offs / sizeof (Entry)); }

                // Return number of slots available to software
                // p = 1 (SW = producer) -> number of slots to produce into (last slot must remain empty)
                // p = 0 (SW = consumer) -> number of slots to consume from
                [[nodiscard]] auto num (unsigned offs, bool p) const { return slot (hwi (offs) - swi - p); }

                // Consume entry and advance index
                [[nodiscard]] Entry const &consume() { return ptr[slot (swi++)]; }

                // Produce entry and advance index
                void produce (Entry const &e) { ptr[slot (swi++)] = e; }

                // Replace entry at hardware offset
                void replace (unsigned offs, Entry &e) const { std::swap (ptr[hwi (offs)], e); }

                // Constructor
                [[nodiscard]] ALWAYS_INLINE explicit Queue (unsigned o) : ord { o + bit_scan_msb (PAGE_SIZE (0) / sizeof (Entry)) }, ptr { new Entry[BIT (ord)] }, swi { 0 } {}
        };

        explicit Smmu (uint64_t p, size_t l, pci_t s) : Mmio { p, l, Memattr::dev() }, sbdf { s } {}

    private:
        static inline constinit Type hwtype { Type::NONE };

        [[nodiscard]] virtual bool init() = 0;
        [[nodiscard]] virtual bool invalidate_tlb (uint16_t) = 0;

        virtual void interrupt() = 0;
};

/*
 * Atomic Specialization for Smmu::Entry
 *
 * Regular atomic semantics do not support operations on the 128-bit integrals used by the
 * SMMU entries. The SMMU storage entries are non-copyable; therefore all readers must go
 * through load to avoid torn reads. At the hardware level, the atomic load is actually a
 * non-modifying write (LOCK CMPXCHG16B).
 *
 * Provides load (atomic 128-bit read) and compare_exchange (atomic 128-bit swap with
 * non-coherent producer flush on success).
 */
template<> class Atomic<Smmu::Entry> final
{
    private:
        Smmu::Entry entry;

    public:
        explicit constexpr Atomic (uint128_t v = 0) : entry { v } {}

        [[nodiscard]] auto load() const { return Smmu::Entry { __sync_val_compare_and_swap (&entry.val, 0, 0) }; }

        [[nodiscard]] bool compare_exchange (Smmu::Entry &o, Smmu::Entry n, bool noncoherent)
        {
            // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878 for the use of __sync intrinsics
            auto const v { __sync_val_compare_and_swap (&entry.val, o.val, n.val) };

            // Update o with the existing value on failure
            if (v != o.val) [[unlikely]] {
                o.val = v;
                return false;
            }

            // Make update visible to non-coherent observers
            Coherence::producer (noncoherent, &entry);

            return true;
        }

        // No copy/move for atomic objects
        Atomic            (Atomic const &) = delete;
        Atomic& operator= (Atomic const &) = delete;
};
