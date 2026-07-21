#ifndef _VECTOR_H
#define _VECTOR_H

typedef struct
{
  void *arr;
  size_t element_size;
  int length, capacity;
}
vector_t;

// For clarity.
// Example declaration:
// VECTOR_T(token_t) *tokens = vector_create(sizeof(token_t), CAPACITY);
#define VECTOR_T(type) vector_t

vector_t *vector_create(size_t element_size, int capacity);

#define VECTOR_ARR(vector, type) ((type*)(vector)->arr)

void *vector_next_element(vector_t *vector);

// Convenience functions for pointer element types.

vector_t *vector_pointer_create(int capacity);

int vector_pointer_append(vector_t *vector, void *elt);

#endif
