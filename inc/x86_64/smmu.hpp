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
#include "hip.hpp"
#include "lock_guard.hpp"
#include "mmio.hpp"
#include "pci.hpp"
#include "status.hpp"
#include "wait.hpp"

class Pd;

class Smmu : public List<Smmu>, protected Mmio
{
    public:
        enum class Type { NONE, AMD, ITL };

        [[nodiscard]] static auto type() { return hwtype; }

        virtual Status assign_dev (Pd *, uintptr_t, bool = true) = 0;

        static void all_interrupt()
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                smmu->interrupt();
        }

        /*
         * Invalidate DEV entry in all SMMUs of the PCI segment group
         */
        [[nodiscard]] static bool seg_invalidate_dev (uint16_t dom, pci_t sbdf)
        {
            bool ret { true };

            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (Pci::seg (smmu->sbdf) == Pci::seg (sbdf))
                    ret &= smmu->invalidate_dev (dom, Pci::bdf (sbdf));

            return ret;
        }

        /*
         * Invalidate INT entry in all SMMUs of the PCI segment group
         */
        [[nodiscard]] static bool seg_invalidate_int (uint16_t seg, uint16_t idx)
        {
            bool ret { true };

            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (Pci::seg (smmu->sbdf) == seg)
                    ret &= smmu->invalidate_int (idx);

            return ret;
        }

        /*
         * Invalidate TLB entry in all SMMUs
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
         * 128-bit Atomic Operations
         */
        class Atomic128
        {
            // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878 for why these use __sync
            public:
                // Atomically try to change 128-bit variable from o to n
                [[nodiscard]] static bool compare_exchange (uint128_t &var, uint128_t &o, uint128_t n)
                {
                    // Initialize e with a stable snapshot of o
                    uint128_t e { o }, v;

                    // Atomically try to change var from e to n
                    if ((v = __sync_val_compare_and_swap (&var, e, n)) == e) [[likely]]
                        return true;

                    // Update o with the existing value on failure
                    o = v;

                    return false;
                }

                // Atomically set 128-bit variable to n and return the prior value
                static void exchange (uint128_t &var, uint128_t &o, uint128_t n)
                {
                    // Initialize e with a guesstimate of 0
                    uint128_t e { 0 }, v;

                    for (;; e = v)
                        if ((v = __sync_val_compare_and_swap (&var, e, n)) == e) [[likely]]
                            break;

                    // Update o with the previous value
                    o = v;
                }
        };

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
                        explicit constexpr Entry (uint64_t l, uint64_t h) : lo { l }, hi { h } {}
                };

                static_assert (__is_standard_layout (Entry) && alignof (Entry) == 16 && sizeof (Entry) == 16);

                Entry *   const ptr;    // Base Pointer
                unsigned  const ord;    // Order of Entries
                index_t         swi;    // Software Index
                Spinlock        lock;   // Queue Lock

                // Return physical base address of queue
                auto base() const { return Kmem::ptr_to_phys (ptr); }

                // Return software offset from software index
                size_t offs() const { return slot (swi) * sizeof (Entry); }

                // Return hardware index from hardware offset (sanitized)
                index_t hwi (unsigned offs) const { return slot (offs / sizeof (Entry)); }

                // Return number of slots available to software
                // p = 1 (SW = producer) -> number of slots to produce into (last slot must remain empty)
                // p = 0 (SW = consumer) -> number of slots to consume from
                index_t num (unsigned offs, bool p) const { return slot (hwi (offs) - swi - p); }

                // Consume entry and advance index
                Entry const &consume() { return ptr[slot (swi++)]; }

                // Produce entry and advance index
                void produce (Entry const &e) { ptr[slot (swi++)] = e; }

                // Replace entry at hardware offset
                void replace (unsigned offs, Entry &e) const { std::swap (ptr[hwi (offs)], e); }

                explicit Queue (Buddy::order_t o) : ptr { static_cast<Entry *>(Buddy::alloc (o)) }, ord { static_cast<unsigned>(bit_scan_msb ((PAGE_SIZE (0) << o) / sizeof (Entry))) }, swi { 0 } {}
        };

        explicit Smmu (uint64_t p, size_t l, pci_t s) : Mmio { p, l, Memattr::dev() }, sbdf { s } {}

    private:
        static inline constinit Type hwtype { Type::NONE };

        virtual bool init() = 0;
        virtual void interrupt() = 0;
        virtual bool invalidate_dev (uint16_t, uint16_t) = 0;
        virtual bool invalidate_int (uint16_t) = 0;
        virtual bool invalidate_tlb (uint16_t) = 0;
};
