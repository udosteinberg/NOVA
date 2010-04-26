/*
 * Reference-Counted Pointer
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "compiler.h"

template <typename T>
class Refptr
{
    private:
        T * const ptr;

    public:
        operator T*() const     { return ptr; }
        T * operator->() const  { return ptr; }

        ALWAYS_INLINE
        inline Refptr (T *p) : ptr (p)
        {
            ptr->add_ref();
        }

        /*
         * If the reference count decreases to 0 then we were the last
         * holder of a reference; in particular the root capability to
         * the object no longer exists and thus new references cannot
         * be obtained; therefore the object can be destroyed immediately.
         */
        ALWAYS_INLINE
        inline ~Refptr()
        {
            if (ptr->del_ref())
                delete ptr;
        }
};
