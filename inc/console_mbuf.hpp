/*
 * Memory-Buffer Console
 *
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

#include "atomic.hpp"
#include "buddy.hpp"
#include "console.hpp"
#include "kmem.hpp"

class Sm;

class Console_mbuf_mmio final
{
    public:
        Atomic<uint32_t, __ATOMIC_RELAXED, __ATOMIC_RELAXED> r_idx { 0 };
        Atomic<uint32_t, __ATOMIC_RELAXED, __ATOMIC_SEQ_CST> w_idx { 0 };

        static constexpr unsigned ord       { 0 };
        static constexpr unsigned size      { BIT (PAGE_BITS + ord) };
        static constexpr unsigned entries   { size - sizeof (r_idx) - sizeof (w_idx) };

        Atomic<char> buffer[entries];

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return Buddy::alloc (ord, Buddy::Fill::BITS0);
        }
};

class Console_mbuf final : private Console
{
    private:
        Sm *sm { nullptr };

        void outc (char) override final;

        Console_mbuf();

    public:
        Console_mbuf_mmio *const regs;

        static Console_mbuf singleton;

        static auto addr() { return Kmem::ptr_to_phys (singleton.regs); }
        static auto size() { return singleton.regs->size; }
};
