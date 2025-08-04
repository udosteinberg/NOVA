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

#include "dc.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Dc::cache { sizeof (Dc), Kobject::alignment };

Dc::Dc (uint64_t t, uint64_t d, uint64_t i) : Kobject { Kobject::Type::DC }, Dc_state { t, d, i }
{
    trace (TRACE_CREATE, "DC:%p created", static_cast<void *>(this));
}
