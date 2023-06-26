/*
 * Reference Counting
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
#include "types.hpp"

class Refcnt
{
    private:
        Atomic<size_t> ref { 0 };

        virtual void collect() {}

    public:
        // Constructor
        Refcnt() = default;

        // No copy/move for reference-counted objects
        Refcnt            (Refcnt const &) = delete;
        Refcnt& operator= (Refcnt const &) = delete;

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

            if (--ref == 0)
                collect();
        }
};

template<typename T> class Refptr final
{
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
        explicit Refptr (T *p) { init (p); }

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
        operator T*() const { return ptr; }

        // Member Selection
        T* operator->() const { return ptr; }

        // Dereference
        T& operator*() const { return *ptr; }

        // Atomic Load
        ALWAYS_INLINE inline auto atomic_load() const { return __atomic_load_n (&ptr, __ATOMIC_ACQUIRE); }

        // Atomic Exchange
        ALWAYS_INLINE inline void atomic_swap (Refptr &r) { __atomic_exchange (&ptr, &r.ptr, &r.ptr, __ATOMIC_SEQ_CST); }
};
