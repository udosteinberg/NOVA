/*
 * Reference Counting
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "assert.hpp"
#include "atomic.hpp"
#include "types.hpp"

class Refcnt
{
    private:
        Atomic<size_t> ref { 0 };

    public:
        // Destructor
        // virtual ~Refcnt() = default;

        // Constructor
        Refcnt() = default;

        // No copy/move for reference-counted objects
        Refcnt            (Refcnt const &) = delete;
        Refcnt& operator= (Refcnt const &) = delete;

        // Increment refcount unless zero or overflowing
        [[nodiscard]] size_t try_inc()
        {
            for (size_t o { ref }, n; n = o + 1, o && n; )
                if (EXPECT_TRUE (ref.compare_exchange (o, n)))
                    return n;

            return 0;
        }

        // Increment refcount unconditionally
        auto ref_inc()
        {
            assert (ref == 0);

            return ++ref;
        }

        // Decrement refcount unconditionally
        auto ref_dec()
        {
            assert (ref != 0);

            return --ref;
        }
};

template <typename T>
class Refptr final
{
    private:
        T* ptr;

        void init (T *p)
        {
            ptr = p && p->try_inc() ? p : nullptr;
        }

        void fini()
        {
            if (EXPECT_TRUE (ptr))
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
            if (EXPECT_TRUE (this != &r)) {
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
};
