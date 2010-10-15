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

#include "avl.h"

Avl *Avl::rotate (Avl *&tree, bool d)
{
    Avl *node;

    node = tree;
    tree = node->lnk[d];
    node->lnk[d] = tree->lnk[!d];
    tree->lnk[!d] = node;

    node->bal = tree->bal = 2;

    return tree->lnk[d];
}

Avl *Avl::rotate (Avl *&tree, bool d, unsigned b)
{
    Avl *node[2];

    node[0] = tree;
    node[1] = node[0]->lnk[d];
    tree = node[1]->lnk[!d];

    node[0]->lnk[d] = tree->lnk[!d];
    node[1]->lnk[!d] = tree->lnk[d];

    tree->lnk[d] = node[1];
    tree->lnk[!d] = node[0];

    tree->bal = node[0]->bal = node[1]->bal = 2;

    if (b == 2)
        return 0;

    node[b != d]->bal = !b;

    return node[b == d]->lnk[!b];
}

bool Avl::insert (Avl **tree, Avl *node)
{
    Avl **p = tree;

    for (Avl *n; (n = *tree); tree = n->lnk + node->larger (n)) {

        if (node->equal (n))
            return false;

        if (!n->balanced())
            p = tree;
    }

    *tree = node;

    Avl *n = *p;

    if (!n->balanced()) {

        bool d1, d2;

        if (n->bal != (d1 = node->larger (n))) {
            n->bal = 2;
            n = n->lnk[d1];
        } else if (d1 == (d2 = node->larger (n->lnk[d1]))) {
            n = rotate (*p, d1);
        } else {
            n = n->lnk[d1]->lnk[d2];
            n = rotate (*p, d1, node->equal (n) ? 2 : node->larger (n));
        }
    }

    for (bool d; n && !node->equal (n); n->bal = d, n = n->lnk[d])
        d = node->larger (n);

    return true;
}

bool Avl::remove (Avl **tree, Avl *node)
{
    Avl **p = tree, **item = 0;
    bool d;

    for (Avl *n; (n = *tree); tree = n->lnk + d) {

        if (node->equal (n))
            item = tree;

        d = node->larger (n);

        if (!n->lnk[d])
            break;

        if (n->balanced() || (n->bal == !d && n->lnk[!d]->balanced()))
            p = tree;
    }

    if (!item)
        return false;

    for (Avl *n; (n = *p); p = n->lnk + d) {

        d = node->larger (n);

        if (!n->lnk[d])
            break;

        if (n->balanced())
            n->bal = !d;

        else if (n->bal == d)
            n->bal = 2;

        else {
            unsigned b = n->lnk[!d]->bal;

            if (b == d)
                rotate (*p, !d, n->lnk[!d]->lnk[d]->bal);
            else {
                rotate (*p, !d);

                if (b == 2) {
                   n->bal = !d;
                   (*p)->bal = d;
                }
            }

            if (n == node)
                item = (*p)->lnk + d;
        }
    }

    Avl *n = *tree;

    *item = n;
    *tree = n->lnk[!d];
    n->lnk[0] = node->lnk[0];
    n->lnk[1] = node->lnk[1];
    n->bal    = node->bal;

    return true;
}
