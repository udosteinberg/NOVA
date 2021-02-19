/*
 * Page Table Entry (ARM)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "ptab.hpp"

template <typename T, typename I, typename O>
class Pte : public Ptab<T, I, O>::Entry
{
    using E = typename Ptab<T, I, O>::Entry;

    protected:
        Pte() = default;
        Pte (E e) : E (e) {}

    public:
        auto type (unsigned l) const { return E::val ? l == T::lev() || (l && (E::val & T::ATTR_nL)) ? E::Type::PTAB : E::Type::LEAF : E::Type::HOLE; }

        static inline void publish() { Barrier::wsb (Barrier::Domain::ISH); }

        // Physical address size
        static inline auto pas (unsigned e)
        {
            static constexpr uint8_t encoding[] { 32, 36, 40, 42, 44, 48, 52, 56 };
            assert (e < sizeof (encoding) / sizeof (*encoding));
            return encoding[e];
        }
};
