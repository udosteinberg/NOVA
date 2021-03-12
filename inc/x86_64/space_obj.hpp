/*
 * Object Space
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

#include "bits.hpp"
#include "buddy.hpp"
#include "capability.hpp"
#include "space.hpp"

class Space_obj final : public Space
{
    private:
        /*
         * CPU A (creating a new Cap Table)             CPU B (walking Cap Tables)
         *
         * (1) ST.RELAXED (init L0-Table)               (3) LD.ACQUIRE (L1-Table[slot])
         * (2) ST.RELEASE (L1-Table[slot] = L0-Table)   (4) LD.RELAXED (use L0-Table)
         *
         * Required Memory Ordering:
         * The stores that initialize a new Cap Table must be observable
         * by remote CPUs before the new Cap Table becomes observable.
         *
         * (1) happens before (2)                       (3) synchronizes with (2)
         *                                              (3) happens before (4)
         */
        using entry_t = Atomic<uintptr_t>;              // ACQUIRE/RELEASE/ACQ_REL

        static constexpr auto lev { 2 };
        static constexpr auto bpl { bit_scan_msb (PAGE_SIZE (0) / sizeof (entry_t)) };

        static auto table (uintptr_t ptr) { return std::launder (reinterpret_cast<Captable *>(ptr)); }

        struct Captable
        {
            static constexpr auto entries { BIT (bpl) };

            entry_t slot[entries] {};

            /*
             * Allocate a Captable
             *
             * @return      Pointer to the Captable (allocation success) or nullptr (allocation failure)
             */
            [[nodiscard]] static void *operator new (size_t) noexcept
            {
                return Buddy::alloc (0);
            }

            /*
             * Deallocate a Captable
             *
             * @param ptr   Pointer to the Captable
             */
            NONNULL static void operator delete (void *ptr)
            {
                Buddy::free (ptr);
            }

            /*
             * Deallocate a Captable subtree
             *
             * @param l     Subtree level
             */
            void deallocate (unsigned l) const
            {
                if (l)
                    for (unsigned i { 0 }; i < entries; i++)
                        if (uintptr_t ptr { slot[i] })
                            table (ptr)->deallocate (l - 1);

                delete this;
            }
        };

        static_assert (sizeof (Captable) == PAGE_SIZE (0));

        entry_t root {};

        Space_obj() : Space { Kobject::Subtype::OBJ }
        {
            insert (Selector::NOVA_OBJ, Capability { this, std::to_underlying (Capability::Perm_sp::TAKE) });
        }

        Space_obj (Refptr<Pd> &ref_pd) : Space { Kobject::Subtype::OBJ, ref_pd } {}

        /*
         * Destructor
         */
        ~Space_obj()
        {
            if (uintptr_t ptr { root }) [[likely]]
                table (ptr)->deallocate (lev - 1);
        }

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: OBJ %p collected", static_cast<void *>(this));
        }

        entry_t *walk (unsigned long, bool);

    public:
        static Space_obj nova;

        static constexpr uint8_t mco { bpl };
        static constexpr uint8_t sbw { bpl * lev };
        static constexpr auto selectors { BIT64 (sbw) };

        enum Selector
        {
            NOVA_CON = selectors - 1,
            NOVA_OBJ = selectors - 2,
            NOVA_HST = selectors - 3,
            NOVA_DMA = selectors - 4,
            NOVA_PIO = selectors - 5,
            NOVA_MSR = selectors - 6,
            ROOT_OBJ = selectors - 7,
            ROOT_HST = selectors - 8,
            ROOT_PIO = selectors - 9,
            ROOT_PD  = selectors - 10,
            NOVA_CPU = 0,
        };

        [[nodiscard]] static Space_obj *create (Status &, Pd *);

        void destroy() override final;

        Capability lookup (unsigned long) const;
        Status     update (unsigned long, Capability);
        Status     insert (unsigned long, Capability);

        Status delegate (Space_obj const *, unsigned long, unsigned long, unsigned, unsigned);
};
