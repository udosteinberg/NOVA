/*
 * Atomic Variables
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

/*
 * Integral Types
 */
template <typename T, int L = __ATOMIC_RELAXED, int S = __ATOMIC_RELAXED>
class Atomic final
{
    private:
        T val;

    public:
        ALWAYS_INLINE inline constexpr Atomic (T v = 0) : val (v) {}

        ALWAYS_INLINE inline T load (int mo = L) const
        {
            T v;
            __atomic_load (&val, &v, mo);
            return v;
        }

        ALWAYS_INLINE inline void store (T v, int mo = S)
        {
            __atomic_store (&val, &v, mo);
        }

        ALWAYS_INLINE inline operator T() const { return load(); }
        ALWAYS_INLINE inline T operator=  (T v) { store (v); return v; }

        ALWAYS_INLINE inline T operator++()     { return __atomic_add_fetch (&val, 1, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator--()     { return __atomic_sub_fetch (&val, 1, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator+= (T v) { return __atomic_add_fetch (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator-= (T v) { return __atomic_sub_fetch (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator^= (T v) { return __atomic_xor_fetch (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator|= (T v) { return __atomic_or_fetch  (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator&= (T v) { return __atomic_and_fetch (&val, v, __ATOMIC_SEQ_CST); }

        ALWAYS_INLINE inline T operator++(int)  { return __atomic_fetch_add (&val, 1, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator--(int)  { return __atomic_fetch_sub (&val, 1, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T fetch_add (T v)  { return __atomic_fetch_add (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T fetch_sub (T v)  { return __atomic_fetch_sub (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T fetch_xor (T v)  { return __atomic_fetch_xor (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T fetch_or  (T v)  { return __atomic_fetch_or  (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T fetch_and (T v)  { return __atomic_fetch_and (&val, v, __ATOMIC_SEQ_CST); }

        ALWAYS_INLINE inline void exchange         (T &o, T &n) {        __atomic_exchange         (&val, &n, &o,                          __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline bool compare_exchange (T &o, T &n) { return __atomic_compare_exchange (&val, &o, &n, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

        ALWAYS_INLINE inline T test_and_set (T v) { return fetch_or   (v) & v; }
        ALWAYS_INLINE inline T test_and_clr (T v) { return fetch_and (~v) & v; }

        Atomic            (Atomic const &) = delete;
        Atomic& operator= (Atomic const &) = delete;
};

/*
 * Pointer Types
 */
template<typename T, int L, int S>
class Atomic<T*, L, S> final
{
    private:
        T* ptr;

    public:
        ALWAYS_INLINE inline constexpr Atomic (T* p = nullptr) : ptr (p) {}

        ALWAYS_INLINE inline T* load (int mo = L) const
        {
            T* p;
            __atomic_load (&ptr, &p, mo);
            return p;
        }

        ALWAYS_INLINE inline void store (T* p, int mo = S)
        {
            __atomic_store (&ptr, &p, mo);
        }

        ALWAYS_INLINE inline operator T*() const    { return load(); }
        ALWAYS_INLINE inline T* operator->() const  { return load(); }
        ALWAYS_INLINE inline T* operator= (T* p)    { store (p); return p; }

        ALWAYS_INLINE inline void exchange         (T* &o, T* &n) {        __atomic_exchange         (&ptr, &n, &o,                          __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline bool compare_exchange (T* &o, T* &n) { return __atomic_compare_exchange (&ptr, &o, &n, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

        Atomic            (Atomic const &) = delete;
        Atomic& operator= (Atomic const &) = delete;
};
