/*
 * Atomic Variables
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

#include "compiler.hpp"
#include "std.hpp"

/*
 * Atomic Type T
 */
template<typename T, int L = __ATOMIC_ACQUIRE, int S = __ATOMIC_RELEASE, int M = __ATOMIC_ACQ_REL> class Atomic;

/*
 * Atomic Integral Type T
 */
template<typename T, int L, int S, int M> requires (std::integral<T>) class Atomic<T, L, S, M> final
{
    private:
        T val;

    public:
        constexpr Atomic() = default;

        constexpr Atomic (T v) : val { v } {}

        // C++ [atomics.types.operations]
        ALWAYS_INLINE inline T load (int o = L) const requires (L != __ATOMIC_ACQ_REL && L != __ATOMIC_RELEASE)
        {
            return __atomic_load_n (&val, o);
        }

        // C++ [atomics.types.operations]
        ALWAYS_INLINE inline void store (T v, int o = S) requires (S != __ATOMIC_ACQ_REL && S != __ATOMIC_ACQUIRE && S != __ATOMIC_CONSUME)
        {
            __atomic_store (&val, &v, o);
        }

        ALWAYS_INLINE inline operator T() const { return load(); }
        ALWAYS_INLINE inline T operator= (T v)  { store (v); return v; }

        ALWAYS_INLINE inline T operator++()     { return __atomic_add_fetch (&val, 1, M); }
        ALWAYS_INLINE inline T operator--()     { return __atomic_sub_fetch (&val, 1, M); }
        ALWAYS_INLINE inline T operator+= (T v) { return __atomic_add_fetch (&val, v, M); }
        ALWAYS_INLINE inline T operator-= (T v) { return __atomic_sub_fetch (&val, v, M); }
        ALWAYS_INLINE inline T operator^= (T v) { return __atomic_xor_fetch (&val, v, M); }
        ALWAYS_INLINE inline T operator|= (T v) { return __atomic_or_fetch  (&val, v, M); }
        ALWAYS_INLINE inline T operator&= (T v) { return __atomic_and_fetch (&val, v, M); }

        ALWAYS_INLINE inline T operator++(int)  { return __atomic_fetch_add (&val, 1, M); }
        ALWAYS_INLINE inline T operator--(int)  { return __atomic_fetch_sub (&val, 1, M); }
        ALWAYS_INLINE inline T fetch_add (T v)  { return __atomic_fetch_add (&val, v, M); }
        ALWAYS_INLINE inline T fetch_sub (T v)  { return __atomic_fetch_sub (&val, v, M); }
        ALWAYS_INLINE inline T fetch_xor (T v)  { return __atomic_fetch_xor (&val, v, M); }
        ALWAYS_INLINE inline T fetch_or  (T v)  { return __atomic_fetch_or  (&val, v, M); }
        ALWAYS_INLINE inline T fetch_and (T v)  { return __atomic_fetch_and (&val, v, M); }

        ALWAYS_INLINE inline void exchange           (T &o, T &n) {        __atomic_exchange           (&val, &n, &o,        M); }
        ALWAYS_INLINE inline T    exchange_n         (      T  n) { return __atomic_exchange_n         (&val,  n,            M); }
        ALWAYS_INLINE inline bool compare_exchange   (T &o, T &n) { return __atomic_compare_exchange   (&val, &o, &n, false, M, __ATOMIC_ACQUIRE); }
        ALWAYS_INLINE inline bool compare_exchange_n (T &o, T  n) { return __atomic_compare_exchange_n (&val, &o,  n, false, M, __ATOMIC_ACQUIRE); }

        ALWAYS_INLINE inline T test_and_set (T v) { return fetch_or   (v) & v; }
        ALWAYS_INLINE inline T test_and_clr (T v) { return fetch_and (~v) & v; }

        Atomic            (Atomic const &) = delete;
        Atomic& operator= (Atomic const &) = delete;
};

/*
 * Atomic Non-Integral Type T
 */
template<typename T, int L, int S, int M> requires (!std::integral<T>) class Atomic<T, L, S, M> final
{
    private:
        T val;

    public:
        constexpr Atomic() = default;

        constexpr Atomic (T v) : val { v } {}

        // C++ [atomics.types.operations]
        ALWAYS_INLINE inline T load (int o = L) const requires (L != __ATOMIC_ACQ_REL && L != __ATOMIC_RELEASE)
        {
            T v;
            __atomic_load (&val, &v, o);
            return v;
        }

        // C++ [atomics.types.operations]
        ALWAYS_INLINE inline void store (T v, int o = S) requires (S != __ATOMIC_ACQ_REL && S != __ATOMIC_ACQUIRE && S != __ATOMIC_CONSUME)
        {
            __atomic_store (&val, &v, o);
        }

        ALWAYS_INLINE inline operator T() const     { return load(); }
        ALWAYS_INLINE inline T operator->() const   { return load(); }
        ALWAYS_INLINE inline T operator= (T v)      { store (v); return v; }

        ALWAYS_INLINE inline void exchange         (T &o, T &n) {        __atomic_exchange         (&val, &n, &o,        M); }
        ALWAYS_INLINE inline bool compare_exchange (T &o, T &n) { return __atomic_compare_exchange (&val, &o, &n, false, M, __ATOMIC_ACQUIRE); }

        Atomic            (Atomic const &) = delete;
        Atomic& operator= (Atomic const &) = delete;
};
