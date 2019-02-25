/*
 * Atomic Operations
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

class Atomic_base
{
    public:
        template <typename T>
        static T load (T &ptr, int memorder = __ATOMIC_SEQ_CST)
        {
            return __atomic_load_n (&ptr, memorder);
        }

        template <typename T>
        static void store (T &ptr, T val, int memorder = __ATOMIC_SEQ_CST)
        {
            __atomic_store_n (&ptr, val, memorder);
        }

        template <typename T>
        static bool cmp_swap (T &ptr, T &o, T n, int memorder = __ATOMIC_SEQ_CST)
        {
            return __atomic_compare_exchange_n (&ptr, &o, n, false, memorder, memorder);
        }

        template <typename T>
        static T exchange (T &ptr, T v, int memorder = __ATOMIC_SEQ_CST)
        {
            return __atomic_exchange_n (&ptr, v, memorder);
        }

        template <typename T>
        static T add (T &ptr, T v, int memorder = __ATOMIC_SEQ_CST)
        {
            return __atomic_add_fetch (&ptr, v, memorder);
        }

        template <typename T>
        static T sub (T &ptr, T v, int memorder = __ATOMIC_SEQ_CST)
        {
            return __atomic_sub_fetch (&ptr, v, memorder);
        }

        template <typename T>
        static void set_mask (T &ptr, T v, int memorder = __ATOMIC_SEQ_CST)
        {
            __atomic_or_fetch (&ptr, v, memorder);
        }

        template <typename T>
        static void clr_mask (T &ptr, T v, int memorder = __ATOMIC_SEQ_CST)
        {
            __atomic_and_fetch (&ptr, ~v, memorder);
        }
};
