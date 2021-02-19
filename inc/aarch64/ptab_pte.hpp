/*
 * Page Table Entry (Arm)
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
#include "ptab.hpp"

template<typename T, typename I, typename O> class Pte : public Ptab<T, I, O>::Entry
{
    using E = typename Ptab<T, I, O>::Entry;

    public:
        auto type (unsigned l) const { return E::val ? l == T::lev() || (l && (E::val & T::ATTR_nL)) ? E::Type::PTAB : E::Type::LEAF : E::Type::HOLE; }

        static void publish() { Barrier::wsb (Barrier::Domain::ISH); }

        // Physical address size
        static constexpr auto pas (unsigned e)
        {
            constexpr unsigned encoding[8] { 32, 36, 40, 42, 44, 48, 52, 56 };
            return encoding[BIT_RANGE (2, 0) & e];
        }
};
