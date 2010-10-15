/*
 * AVL Tree
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

class Avl
{
    private:
        Avl *       lnk[2];
        unsigned    bal;

        bool balanced() const { return bal == 2; }

        static Avl *rotate (Avl *&, bool);
        static Avl *rotate (Avl *&, bool, unsigned);

    protected:
        virtual bool larger (Avl *x) const = 0;
        virtual bool equal  (Avl *x) const = 0;

        Avl *link (bool d) const { return lnk[d]; }

        explicit Avl() : bal (2) { lnk[0] = lnk[1] = 0; }

    public:
        static bool insert (Avl **, Avl *);
        static bool remove (Avl **, Avl *);
};
