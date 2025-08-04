/*
 * Device Context (DC)
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "dc_state.hpp"
#include "kobject.hpp"
#include "status.hpp"

class Dc final : public Kobject, public Dc_state
{
    private:
        static Slab_cache cache;

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: DC %p collected", static_cast<void *>(this));
        }

    public:
        explicit Dc (uint64_t, uint64_t = 0, uint64_t = 0);

        [[nodiscard]] static Dc *create (Status &s, uint64_t t, uint64_t d, uint64_t i)
        {
            auto const dc { new (cache) Dc { t, d, i } };

            if (!dc) [[unlikely]]
                s = Status::MEM_OBJ;

            return dc;
        }

        void destroy()
        {
            this->~Dc();

            operator delete (this, cache);
        }
};
