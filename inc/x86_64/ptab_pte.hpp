/*
 * Page Table Entry (x86)
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

#include "ptab.hpp"

template <typename T, typename I, typename O>
class Pte : public Ptab<T, I, O>::Entry
{
    using E = typename Ptab<T, I, O>::Entry;

    protected:
        Pte() = default;
        Pte (E e) : E (e) {}

    public:
        auto type (unsigned l) const { return E::val ? l && !(E::val & T::ATTR_S) ? E::Type::PTAB : E::Type::LEAF : E::Type::HOLE; }

        static inline void publish() {}
};
