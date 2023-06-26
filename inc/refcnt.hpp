/*
 * Reference Counting
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "assert.hpp"
#include "atomic.hpp"

class Refcnt
{
    private:
        Atomic<size_t> ref { 0 };

        virtual void collect() = 0;

    protected:
        // Constructor
        Refcnt() = default;

        // Destructor
        ~Refcnt() = default;

        // No copy/move for reference-counted objects
        Refcnt            (Refcnt const &) = delete;
        Refcnt& operator= (Refcnt const &) = delete;

        [[nodiscard]] bool dead() const { return ref == 0; }

    public:
        // Increment refcount unless zero or overflowing
        [[nodiscard]] size_t try_inc()
        {
            for (size_t o { ref }, n; n = o + 1, o && n; )
                if (ref.compare_exchange (o, n)) [[likely]]
                    return n;

            return 0;
        }

        // Increment refcount unconditionally
        void ref_inc()
        {
            assert (ref == 0);

            ++ref;
        }

        // Decrement refcount unconditionally
        void ref_dec()
        {
            assert (ref != 0);

            // Invoke callback function when refcount becomes zero
            if (--ref == 0) [[unlikely]]
                collect();
        }
};

template<typename T> class Refptr final
{
    template<typename, int, int, int> friend class Atomic;

    private:
        T* ptr;

        void init (T *p)
        {
            ptr = p && p->try_inc() ? p : nullptr;
        }

        void fini()
        {
            if (ptr) [[likely]]
                ptr->ref_dec();
        }

        void xfer (Refptr &r)
        {
            ptr = r.ptr;
            r.ptr = nullptr;
        }

    public:
        // Destructor
        ~Refptr() { fini(); }

        // Constructor
        explicit constexpr Refptr() : ptr { nullptr } {}

        // Constructor
        [[nodiscard]] explicit Refptr (T *p) { init (p); }

        // Copy Constructor
        Refptr (Refptr const &) = delete;

        // Copy Assignment
        Refptr& operator= (Refptr const &) = delete;

        // Move Constructor
        Refptr (Refptr&& r) { xfer (r); }

        // Move Assignment
        Refptr& operator= (Refptr&& r)
        {
            if (this != &r) [[likely]] {
                fini();
                xfer (r);
            }

            return *this;
        }

        // Conversion
        constexpr operator T*() const { return ptr; }

        // Member Selection
        constexpr T* operator->() const { return ptr; }

        // Dereference
        constexpr T& operator*() const { return *ptr; }
};

/*
 * Atomic Specialization for Refptr<T>
 *
 * Regular byte-copy atomic semantics do not preserve refcount ownership across the edit.
 * Every operation must surface the displaced reference at a location the caller controls
 * so that the refcount stays balanced.
 *
 * Provides load (non-owning raw pointer read), exchange (refcount-neutral swap) and
 * compare_exchange with ownership-handoff on success. Store is omitted - its API has
 * no return channel to surface the displaced reference.
 */
template<typename T, int L, int S, int M> requires (Valid_MO<L, S, M>) class Atomic<Refptr<T>, L, S, M> final
{
    private:
        Refptr<T> val;

    public:
        constexpr Atomic() = default;

        ALWAYS_INLINE inline auto load (int m = L) const
        {
            return __atomic_load_n (&val.ptr, m);
        }

        ALWAYS_INLINE inline void exchange (Refptr<T> &n)
        {
            n.ptr = __atomic_exchange_n (&val.ptr, n.ptr, M);
        }

        ALWAYS_INLINE inline bool compare_exchange (T *&o, Refptr<T> &n)
        {
            if (__atomic_compare_exchange_n (&val.ptr, &o, n.ptr, false, M, L)) [[likely]] {
                n.ptr = o;
                return true;
            }

            return false;
        }

        // No copy/move for atomic objects
        Atomic            (Atomic const &) = delete;
        Atomic& operator= (Atomic const &) = delete;
};
