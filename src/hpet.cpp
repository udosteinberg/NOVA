/*
 * High Precision Event Timer (HPET)
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

#include "hpet.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Hpet::cache (sizeof (Hpet), 8);

Hpet *Hpet::list;

Hpet::Hpet (Paddr p, unsigned i) : phys (p), id (i), next (nullptr), rid (0)
{
    Hpet **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;
}
