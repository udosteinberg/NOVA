/*
 * Timeout
 *
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
    protected:
        Timeout *prev, *next;
        uint64 time;

        virtual void trigger() = 0;

    public:
        static Timeout *list CPULOCAL;

        ALWAYS_INLINE
        inline Timeout() : prev (nullptr), next (nullptr), time (0) {}

        ALWAYS_INLINE
        inline bool active() const { return prev || list == this; }

        void enqueue (uint64);
        uint64 dequeue();

        static void check();
};
