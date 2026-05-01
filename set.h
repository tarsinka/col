#ifndef SET_H
#define SET_H

#include "vector.h"

typedef struct set set;

struct set
{
    unsigned cardinal;

    int (*order)(void const*, void const*);
    vector *vec;
};

set *init_set(size_t const, void(*)(void *, void const *), int (*)(void const *, void const *));
void free_set(set *);
void copy_set(set *, set const *);

void add(set *, void const *);
bool contains(set *, void const *);

int ord(unsigned const *, unsigned const *);

#endif