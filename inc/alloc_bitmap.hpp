/*
 * Bitmap-Based Allocator
 *
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

#include "bitmap.hpp"

/*
 * Bitmap Allocator with B Bits
 */
template<size_t B> class Alloc_bitmap final : private Bitmap<B, false>
{
    private:
        using Base = Bitmap<B, false>;

        // Constraints
        static_assert (B % Base::cnt == 0, "Bitmap size must be a multiple of bitmap_t bits");

        // Scan words
        size_t words { B / Base::cnt };

        // Scan start hint
        Atomic<size_t, __ATOMIC_RELAXED, __ATOMIC_RELAXED, __ATOMIC_RELAXED> hint { 0 };

    public:
        // Smallest unsigned integer type that can hold an allocator index [0, B)
        using index_t = std::conditional_t<(B <= BITN (type_bits< uint8_t>())),  uint8_t,
                        std::conditional_t<(B <= BITN (type_bits<uint16_t>())), uint16_t,
                        std::conditional_t<(B <= BITN (type_bits<uint32_t>())), uint32_t, void>>>;

        struct Result
        {
            size_t v;
            constexpr explicit operator bool() const { return v != B; }
            constexpr auto val() const { assert (*this); return static_cast<index_t>(v); }
        };

        /*
         * Reduce allocator capacity to b <= B bits in the boot phase
         * Not concurrency-safe; any prior allocation must have index < b
         */
        void reduce (size_t b)
        {
            auto const w { aligned_up (Base::cnt, b) / Base::cnt };

            if (w <= words) [[likely]] {

                words = w;

                // Reserve trailing bits beyond b
                if (auto const t { b % Base::cnt }) [[unlikely]]
                    Base::bitmap[words - 1].val |= Base::bp1 << t;
            }
        }

        [[nodiscard]] Result alloc()
        {
            for (size_t i { 0 }, w { hint }; i < words; i++, w = w + 1 < words ? w + 1 : 0) {
                for (typename Base::bitmap_t v; (v = Base::bitmap[w].val) != Base::bp1; ) {
                    auto const b { Base::bit (w, bit_scan_lsb (~v)) };
                    if (!Base::tas (b)) {
                        hint = w;
                        return { b };
                    }
                }
            }

            // Sentinel B means allocation failure
            return { B };
        }

        void free (index_t b) { Base::clr (b); }

        void reserve (index_t b) { Base::set (b); }
};
