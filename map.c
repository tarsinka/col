#include "map.h"
#include <stdlib.h>
#include <stdio.h>

map *init_map(size_t offset, void (*copy)(void *, void const *))
{
    map *m = malloc(sizeof(map));

    m->size = 0;
    m->capacity = DEFAULT_MAP_CAPACITY;
    m->offset = offset;

    m->copy = copy;

    m->alpha_max = DEFAULT_ALPHA_MAX;
    m->buckets = malloc(m->capacity * sizeof(struct bucket *));

    for(unsigned u = 0; u < m->capacity; u++) m->buckets[u] = init_bucket(offset, copy);

    return m;
}

void free_map(map *m)
{
    for(unsigned u = 0; u < m->capacity; u++) free_bucket(m->buckets[u]);
    free(m);
}

void copy_map(map *dst, map const *src)
{
    dst->size = src->size;
    dst->capacity = src->capacity;
    dst->offset = src->offset;
    dst->alpha_max = src->alpha_max;

    dst->buckets = malloc(dst->capacity * sizeof(struct bucket *));
    for(unsigned u = 0; u < dst->capacity; u++)
    {
        copy_bucket(dst->buckets[u], src->buckets[u]);
    }
}

struct bucket *init_bucket(size_t offset, void (*copy)(void *, void const*))
{
    struct bucket *b = malloc(sizeof(struct bucket));

    b->keys = init_vector(sizeof(unsigned), NULL);
    b->vals = init_vector(offset, copy);

    return b;
}

void free_bucket(struct bucket *b)
{
    free_vector(b->keys);
    free_vector(b->vals);
}

void copy_bucket(struct bucket *dst, struct bucket const *src)
{
    copy_vector(dst->keys, src->keys);
    copy_vector(dst->vals, src->vals);
}

void bind(map *m, unsigned const key, void const *val)
{
    unsigned hashkey = HASH(m, key);
    struct bucket *b = m->buckets[hashkey];
    
    push_back(b->keys, &key);
    push_back(b->vals, val);
    
    m->size++;

    if(LOAD(m) > m->alpha_max) rehash(m);
}

void *find(map *m, unsigned const key)
{
    unsigned hashkey = HASH(m, key);
    struct bucket *b = m->buckets[hashkey];

    for(unsigned u = 0; u < size(b->keys); u++)
    {
        if(*((unsigned *) at(b->keys, u)) == key) return at(b->vals, u);
    }

    return NULL;
}

bool exists(map *m, unsigned const key)
{
    unsigned hashkey = HASH(m, key);
    struct bucket *b = m->buckets[hashkey];

    for(unsigned u = 0; u < size(b->keys); u++)
    {
        if(*((unsigned *) at(b->keys, u)) == key) return true;
    }

    return false;
}

void rehash(map *m)
{
    unsigned old_capacity = m->capacity;
    struct bucket **old_buckets = m->buckets;

    m->capacity *= 2;
    m->buckets = malloc(m->capacity * sizeof(struct bucket *));

    for(unsigned u = 0; u < m->capacity; u++) m->buckets[u] = init_bucket(m->offset, m->copy);
    
    for(unsigned u = 0; u < old_capacity; u++)
    {
        struct bucket *b = old_buckets[u];
        for(unsigned u = 0; u < size(b->keys); u++)
            bind(m, *((unsigned *) at(b->keys, u)), at(b->vals, u));
    }

    for(unsigned u = 0; u < old_capacity; u++) free_bucket(old_buckets[u]);
    free(old_buckets);
}