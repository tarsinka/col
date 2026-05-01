#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vector.h"

vector *init_vector(size_t const offset, void (*copy)(void *, void const *))
{
    vector *vec = malloc(sizeof(vector));
    
    vec->capacity = DEFAULT_VECTOR_CAPACITY;
    vec->offset = offset;
    vec->copy = copy;

    vec->begin = malloc(vec->offset * vec->capacity);
    vec->end = vec->begin;

    memset(vec->begin, 0x0, vec->offset * vec->capacity);

    return vec;
}

void free_vector(vector *vec)
{
    free(vec->begin);
    free(vec);
}

void resize(vector *vec)
{
    size_t curr_size = size(vec);
    vec->capacity *= 2;

    vec->begin = realloc(vec->begin, vec->capacity * vec->offset);
    vec->end = vec->begin + curr_size * vec->offset;

    memset(vec->end, 0x0, vec->offset * (vec->capacity - size(vec)));
}

void copy_vector(vector *dst, vector const *src)
{
    dst->capacity = src->capacity;
    dst->offset = src->offset;
    dst->copy = src->copy;

    dst->begin = malloc(dst->offset * dst->capacity);
    dst->end = dst->begin;

    memset(dst->begin, 0x0, dst->offset * dst->capacity);
    memcpy(dst->begin, src->begin, size(src) * dst->offset);
}

void push_back(vector *vec, void const *el)
{
    if(size(vec) >= vec->capacity) resize(vec);
    
    if(vec->copy != NULL) vec->copy(vec->end, el);
    else memcpy(vec->end, el, vec->offset);

    vec->end += vec->offset;
}

void pop_back(vector *vec)
{
    if(vec->end == vec->begin) return;

    vec->end -= vec->offset;
    memset(vec->end, 0x0, vec->offset);
}

void insert(vector *vec, size_t const pos, void const *el)
{
    if(size(vec) >= vec->capacity) resize(vec);

    memmove(vec->begin + (pos + 1) * vec->offset, vec->begin + pos * vec->offset, (size(vec) - pos) * vec->offset);
    memcpy(vec->begin + pos * vec->offset, el, vec->offset);

    vec->end += vec->offset;
}

void *at(vector *vec, size_t const pos)
{
    if(pos >= vec->capacity) return NULL;
    return vec->begin + pos * vec->offset;
}

void *front(vector *vec)
{
    return vec->begin;
}

void *back(vector *vec)
{
    if(!size(vec)) return NULL;
    return vec->end - vec->offset;
}

size_t size(vector const *vec)
{
    return (vec->end - vec->begin) / vec->offset;
}