/*
 * AVL Tree
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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
        Avl *       ptr[2];
        unsigned    bal;

        bool balanced() const { return bal == 2; }

        static Avl *rotate (Avl *&, bool);
        static Avl *rotate (Avl *&, bool, unsigned);

    protected:
        virtual bool larger (Avl *x) const;
        virtual bool equal  (Avl *x) const;

        Avl *link (bool d) const { return ptr[d]; }

        explicit Avl() : bal (2) { ptr[0] = ptr[1] = 0; }

    public:
        static bool insert (Avl **, Avl *);
        static bool remove (Avl **, Avl *);
};
