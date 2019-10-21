/*
 * Memory Buffer Console
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "buddy.hpp"
#include "console.hpp"

class Sm;

class Console_mbuf_mmio
{
    public:
        uint32 r_ptr { 0 };
        uint32 w_ptr { 0 };

        static unsigned const ord = 0;
        static unsigned const size = BIT (ord + PAGE_BITS);
        static unsigned const entries = size - sizeof (r_ptr) - sizeof (w_ptr);

        char buffer[entries];

        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (ord);
        }
};

class Console_mbuf final : private Console
{
    private:
        Console_mbuf_mmio *regs { nullptr };
        Sm *sm                  { nullptr };

        bool fini() override final { return false; }
        void outc (char) override final;

    public:
        static Console_mbuf con;

        Console_mbuf();

        uint64 addr() const { return Buddy::ptr_to_phys (regs); }
        uint64 size() const { return regs->size; }
};
