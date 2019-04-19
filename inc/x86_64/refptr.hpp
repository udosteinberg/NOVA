/*
 * Reference-Counted Pointer
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "atomic.hpp"

class Refcount
{
    private:
        uint32 ref;

    public:
        ALWAYS_INLINE
        inline Refcount() : ref (1) {}

        ALWAYS_INLINE
        inline bool add_ref()
        {
            for (uint32 r; (r = ref); )
                if (Atomic::cmp_swap (ref, r, r + 1))
                    return true;

            return false;
        }

        ALWAYS_INLINE
        inline bool del_ref()
        {
            return Atomic::sub (ref, 1U) == 0;
        }
};

template <typename T>
class Refptr
{
    private:
        T * const ptr;

    public:
        operator T*() const     { return ptr; }
        T * operator->() const  { return ptr; }

        ALWAYS_INLINE
        inline Refptr (T *p) : ptr (p->add_ref() ? p : nullptr) {}

        ALWAYS_INLINE
        inline ~Refptr()
        {
            if (ptr && ptr->del_ref())
                delete ptr;
        }
};
