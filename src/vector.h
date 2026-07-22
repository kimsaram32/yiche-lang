#ifndef YICHE_VECTOR_H
#define YICHE_VECTOR_H

typedef struct
{
  void *arr;
  size_t element_size;
  int length, capacity;
}
vector_t;

// To promote clarity, the macro VECTOR_T(type) is preferred in type specifiers
// whenever possible.
//
// Example usage:
// VECTOR_T(token_t) *tokens = vector_create(sizeof(token_t), CAPACITY);
#define VECTOR_T(type) vector_t

vector_t *vector_create(size_t element_size, int capacity);

void vector_free(vector_t *vector);

#define VECTOR_ARR(vector, type) ((type*)(vector)->arr)

// Increment the vector's length by 1, and return the pointer to the last
// element.
//
// Previously obtained pointers to elements may become invalid after the
// function returns.
void *vector_next_element(vector_t *vector);

// Convenience functions for pointer element types.

VECTOR_T(void*) *vector_pointer_create(int capacity);

int vector_pointer_append(VECTOR_T(void*) *vector, void *elt);

#endif
