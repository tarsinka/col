#include "utils.h"

int ord_pair(pair const *p, pair const *q)
{
    if(p->fst == q->fst && p->snd == q->snd)
        return 0;

    if(p->fst < q->fst)
        return -1;

    return 1;
}