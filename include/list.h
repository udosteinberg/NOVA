/*
 * List Element
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

#include "compiler.h"

template <typename T>
class List
{
    protected:
        T *next;

    public:
        ALWAYS_INLINE
        List (T *&list) : next (nullptr)
        {
            T **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = static_cast<T *>(this);
        }
};
