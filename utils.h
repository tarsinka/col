#ifndef UTILS_H
#define UTILS_H

typedef struct pair pair;

struct pair
{
    unsigned fst;
    unsigned snd;
};

int ord_pair(pair const *, pair const *);

#endif