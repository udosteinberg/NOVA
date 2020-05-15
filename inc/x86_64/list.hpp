/*
 * Linked List
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

template <typename T>
class List
{
    protected:
        T *next { nullptr };

        void insert (T *&list)
        {
            auto ptr { &list };

            while (*ptr)
                ptr = &(*ptr)->next;

            *ptr = static_cast<T *>(this);
        }

        void remove (T *&list)
        {
            auto ptr { &list };

            while (*ptr != this)
                ptr = &(*ptr)->next;

            *ptr = next;
            next = nullptr;
        }

    public:
        List (T *&list)
        {
            insert (list);
        }
};
