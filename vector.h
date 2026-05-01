#ifndef VECTOR_H
#define VECTOR_H

#define DEFAULT_VECTOR_CAPACITY 16 

#include <stddef.h>

typedef struct vector vector;

struct vector
{
    size_t capacity;
    size_t offset;

    void (*copy)(void *, void const *);

    void *begin;
    void *end;
};

vector *init_vector(size_t const, void(*)(void *, void const *));
void resize(vector *);
void copy_vector(vector *, vector const *);
void free_vector(vector *);

void *at(vector *, size_t const);
void *front(vector *);
void *back(vector *);

size_t size(vector const *);

void push_back(vector *, void const *);
void pop_back(vector *);
void insert(vector *, size_t const, void const *);

#endif