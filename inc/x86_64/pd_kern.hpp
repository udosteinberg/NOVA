/*
 * Kernel Protection Domain (PD)
 *
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

#include "pd.hpp"

class Pd_kern final : private Pd
{
    private:
        Pd_kern();

        void update_user_mem (uint64, size_t, bool);
        void update_user_pio (uint64, size_t, bool);

        static Pd_kern singleton;

    public:
        static inline Pd &nova() { return singleton; }

        static inline void insert_user_obj (unsigned long sel, Capability cap) { singleton.Space_obj::insert (sel, cap); }

        static inline void insert_user_mem (uint64 p, size_t s) { singleton.update_user_mem (p, s, true);  }
        static inline void remove_user_mem (uint64 p, size_t s) { singleton.update_user_mem (p, s, false); }

        static inline void insert_user_pio (uint64 p, size_t s) { singleton.update_user_pio (p, s, true);  }
        static inline void remove_user_pio (uint64 p, size_t s) { singleton.update_user_pio (p, s, false); }
};
