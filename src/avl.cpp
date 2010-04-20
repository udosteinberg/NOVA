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

#include "avl.h"

Avl *Avl::rotate (Avl *&tree, bool d)
{
    Avl *node;

    node = tree;
    tree = node->ptr[d];
    node->ptr[d] = tree->ptr[!d];
    tree->ptr[!d] = node;

    node->bal = tree->bal = 2;

    return tree->ptr[d];
}

Avl *Avl::rotate (Avl *&tree, bool d, unsigned b)
{
    Avl *node[2];

    node[0] = tree;
    node[1] = node[0]->ptr[d];
    tree = node[1]->ptr[!d];

    node[0]->ptr[d] = tree->ptr[!d];
    node[1]->ptr[!d] = tree->ptr[d];

    tree->ptr[d] = node[1];
    tree->ptr[!d] = node[0];

    tree->bal = node[0]->bal = node[1]->bal = 2;

    if (b == 2)
        return 0;

    node[b != d]->bal = !b;

    return node[b == d]->ptr[!b];
}

bool Avl::insert (Avl **tree, Avl *node)
{
    Avl **p = tree;

    for (Avl *n; (n = *tree); tree = n->ptr + node->larger (n)) {

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
            n = n->ptr[d1];
        } else if (d1 == (d2 = node->larger (n->ptr[d1]))) {
            n = rotate (*p, d1);
        } else {
            n = n->ptr[d1]->ptr[d2];
            n = rotate (*p, d1, node->equal (n) ? 2 : node->larger (n));
        }
    }

    for (bool d; n && !node->equal (n); n->bal = d, n = n->ptr[d])
        d = node->larger (n);

    return true;
}

bool Avl::remove (Avl **tree, Avl *node) {

    Avl **p = tree, **item = 0;
    bool d;

    for (Avl *n; (n = *tree); tree = n->ptr + d) {

        if (node->equal (n))
            item = tree;

        d = node->larger (n);

        if (!n->ptr[d])
            break;

        if (n->balanced() || (n->bal == !d && n->ptr[!d]->balanced()))
            p = tree;
    }

    if (!item)
        return false;

    for (Avl *n; (n = *p); p = n->ptr + d) {

        d = node->larger (n);

        if (!n->ptr[d])
            break;

        if (n->balanced())
            n->bal = !d;

        else if (n->bal == d)
            n->bal = 2;

        else {
            unsigned b = n->ptr[!d]->bal;

            if (b == d)
                rotate (*p, !d, n->ptr[!d]->ptr[d]->bal);
            else {
                rotate (*p, !d);

                if (b == 2) {
                   n->bal = !d;
                   (*p)->bal = d;
                }
            }

            if (n == node)
                item = (*p)->ptr + d;
        }
    }


    Avl *n = *tree;

    *item = n;
    *tree = n->ptr[!d];
    n->ptr[0] = node->ptr[0];
    n->ptr[1] = node->ptr[1];
    n->bal    = node->bal;

    return true;
}
