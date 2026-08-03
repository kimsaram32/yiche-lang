#ifndef YICHE_VECTOR_H
#define YICHE_VECTOR_H

typedef struct
{
  void *arr;
  size_t element_size;
  int length, capacity;
  destructor_t destructor;
}
vector_t;

// The macro 'VECTOR_T(type)' denotes a vector with element type of 'type'. The
// size of an element of such vector must equal to 'sizeof(type)'.
//
// To promote clarity, this macro is preferred in type specifiers whenever
// possible.
//
// Example usage:
// VECTOR_T(token_t) *tokens = vector_create(sizeof(token_t), ...);
#define VECTOR_T(type) vector_t

// 'destructor' takes a pointer to an element and should perform proper cleanup
// on that memory region, e.g., free the object pointed by its member, if it's a
// structure type. It should not free the memory region itself.
vector_t *vector_create(size_t element_size, int capacity, destructor_t destructor);

void vector_free(vector_t *vector);

#define VECTOR_ARR(vector, type) ((type*)(vector)->arr)

// Increment the vector's length by 1, and copy 'vector->element_size' bytes
// from 'elt' to the last element. On success, '1' is returned. If an
// out-of-memory error occurs, '0' is returned instead.
//
// Previously obtained pointers to elements may become invalid after the
// function returns.
int vector_append(vector_t *vector, void *elt);

#endif
