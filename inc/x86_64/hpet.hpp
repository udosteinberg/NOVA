/*
 * High Precision Event Timer (HPET)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "list.hpp"
#include "slab.hpp"

class Hpet final : public List<Hpet>
{
    private:
        uint8_t const   id;                                     // Enumeration ID
        pci_t           pci                 { 0 };              // PCI S:B:D:F

        static inline   Hpet *      list    { nullptr };        // HPET List
        static          Slab_cache  cache;                      // HPET Slab Cache

    public:
        explicit Hpet (uint8_t i) : List { list }, id { i } {}

        static bool claim_dev (pci_t p, uint8_t i)
        {
            for (auto l { list }; l; l = l->next)
                if (l->pci == 0 && l->id == i) {
                    l->pci = p;
                    return true;
                }

            return false;
        }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }
};
