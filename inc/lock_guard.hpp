/*
 * Lock Guard
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.hpp"

template <typename T>
class Lock_guard final
{
    private:
        T &lock;

    public:
        ALWAYS_INLINE
        Lock_guard (T &l) : lock (l) { lock.lock(); }

        ALWAYS_INLINE
        ~Lock_guard() { lock.unlock(); }

        Lock_guard            (Lock_guard const &) = delete;
        Lock_guard& operator= (Lock_guard const &) = delete;
};
