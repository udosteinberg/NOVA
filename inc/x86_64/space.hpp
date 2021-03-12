/*
 * Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "pd.hpp"

class Space : public Kobject
{
    private:
        Refptr<Pd> const pd;

    protected:
        Space (Kobject::Subtype s) : Kobject { Kobject::Type::PD, s }, pd { &Pd::nova } {}

        Space (Kobject::Subtype s, Refptr<Pd> &ref_pd) : Kobject { Kobject::Type::PD, s }, pd { std::move (ref_pd) } {}

    public:
        // Architecturally supported spaces must override these defaults in the derived class
        static constexpr uint8_t mco { 0 };
        static constexpr uint8_t sbw { 0 };

        Pd *get_pd() const { return pd; }
};
