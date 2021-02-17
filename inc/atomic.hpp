/*
 * Atomic Variables
 *
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

/*
 * Integral Types
 */
template <typename T>
class Atomic final
{
    private:
        T   val;

        ALWAYS_INLINE inline T load() const
        {
            T v;
            __atomic_load (&val, &v, __ATOMIC_RELAXED);
            return v;
        }

        ALWAYS_INLINE inline void store (T v)
        {
            __atomic_store (&val, &v, __ATOMIC_RELAXED);
        }

    public:
        ALWAYS_INLINE inline constexpr Atomic (T v) : val (v) {}

        ALWAYS_INLINE inline operator T() const { return load(); }
        ALWAYS_INLINE inline T operator=  (T v) { store (v); return v; }
        ALWAYS_INLINE inline T operator|= (T v) { return __atomic_or_fetch  (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator&= (T v) { return __atomic_and_fetch (&val, v, __ATOMIC_SEQ_CST); }
        ALWAYS_INLINE inline T operator++()     { return __atomic_add_fetch (&val, 1, __ATOMIC_SEQ_CST); }

        Atomic            (const Atomic&) = delete;
        Atomic& operator= (const Atomic&) = delete;
};

/*
 * Pointer Types
 */
template<typename T>
class Atomic<T*> final
{
    private:
        T*  val;

        ALWAYS_INLINE inline T* load() const
        {
            T* v;
            __atomic_load (&val, &v, __ATOMIC_RELAXED);
            return v;
        }

        ALWAYS_INLINE inline void store (T* v)
        {
            __atomic_store (&val, &v, __ATOMIC_RELAXED);
        }

    public:
        ALWAYS_INLINE inline constexpr Atomic (T* v) : val (v) {}

        ALWAYS_INLINE inline operator T*() const { return load(); }
        ALWAYS_INLINE inline T* operator->()     { return load(); }
        ALWAYS_INLINE inline T* operator= (T* v) { store (v); return v; }

        Atomic            (const Atomic&) = delete;
        Atomic& operator= (const Atomic&) = delete;
};
