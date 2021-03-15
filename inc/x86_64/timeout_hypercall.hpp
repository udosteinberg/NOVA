/*
 * Hypercall Timeout
 *
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "timeout.hpp"

class Ec;
class Sm;

class Timeout_hypercall final : public Timeout
{
    private:
        Ec * const  ec                      { nullptr };
        Sm *        sm                      { nullptr };

        void trigger() override;

    public:
        ALWAYS_INLINE
        inline Timeout_hypercall (Ec *e) : ec (e) {}

        ALWAYS_INLINE
        inline void enqueue (uint64 t, Sm *s) { sm = s; Timeout::enqueue (t); }
};
