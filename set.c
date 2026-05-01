#include "set.h"
#include <stdlib.h>
#include <stdio.h>

set *init_set(size_t const offset, void (*copy)(void *, void const *), int (*order)(void const*, void const*))
{
    set *s = malloc(sizeof(set));

    s->cardinal = 0;
    s->order = order;
    s->vec = init_vector(offset, copy);

    return s;
}

void free_set(set *s)
{
    free_vector(s->vec);
    free(s);
}

void copy_set(set *dst, set const *src)
{
    dst->cardinal = src->cardinal;
    dst->order = src->order;
    copy_vector(dst->vec, src->vec);
}

unsigned find_pos(set *s, void const *val)
{
    size_t beg_pos = 0;
    size_t end_pos = size(s->vec);

    while(end_pos - beg_pos > 1) {
        size_t mid_pos = (beg_pos + end_pos) / 2;
        unsigned *mid_el = at(s->vec, mid_pos);
        int ord = s->order(mid_el, val);

        switch(ord)
        {
            case -1:
                beg_pos = mid_pos;
                break;
            case 0:
                beg_pos = mid_pos;
                end_pos = mid_pos;
                break;
            case 1:
                end_pos = mid_pos;
                break;
        }
    }

    return beg_pos;
}

void add(set *s, void const *val)
{
    insert(s->vec, find_pos(s, val), val);
    s->cardinal = size(s->vec);
}

bool contains(set *s, void const *el)
{
    return s->order(at(s->vec, find_pos(s, el)), el);
}

int ord(unsigned const *u, unsigned const *v)
{
    if(*u < *v)
        return -1;
    else if(*u == *v)
        return 0;
    else
        return 1;
}