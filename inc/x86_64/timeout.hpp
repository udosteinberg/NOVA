/*
 * Timeout
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

#include "compiler.hpp"
#include "types.hpp"

class Timeout
{
    private:
        uint64              time            { 0 };
        Timeout *           prev            { nullptr };
        Timeout *           next            { nullptr };

        static Timeout *    list            CPULOCAL;

        virtual void trigger() = 0;

    public:
        // Enforce a constructor for CPU-local timeouts
        ALWAYS_INLINE
        inline Timeout() {}

        void enqueue (uint64);
        uint64 dequeue();

        static void check();
        static void sync();
};
