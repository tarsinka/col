#ifndef MAP_H
#define MAP_H

#include "vector.h"

#define DEFAULT_MAP_CAPACITY 16
#define DEFAULT_ALPHA_MAX 1.5

#define HASH(m,key) key%m->capacity
#define LOAD(m) (float)m->size/(float)m->capacity

typedef struct map map;

struct bucket
{
    vector *keys;
    vector *vals;
};

struct map
{
    size_t size;
    size_t capacity;
    size_t offset;

    void (*copy)(void *, void const *);

    float alpha_max;

    struct bucket **buckets;
};

struct bucket *init_bucket(size_t, void(*)(void *, void const *));
void free_bucket(struct bucket *);
void copy_bucket(struct bucket *, struct bucket const *);

map *init_map(size_t, void(*)(void *, void const *));
void free_map(map *);
void copy_map(map *, map const *);

void bind(map *, unsigned const, void const *);
void *find(map *, unsigned const);
void rehash(map *);

bool exists(map *, unsigned const);

#endif