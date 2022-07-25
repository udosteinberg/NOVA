/*
 * Space
 *
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

#include "atomic.hpp"
#include "kobject.hpp"
#include "status.hpp"

class Pd;
class Space_hst;

class Space : public Kobject
{
    private:
        Pd *const pd;

    protected:
        inline Space (Kobject::Subtype s, Pd *p) : Kobject (Kobject::Type::PD, s), pd (p) {}

    public:
        inline auto get_pd() const { return pd; }
};
