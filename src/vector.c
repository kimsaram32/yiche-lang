#include <stdlib.h>
#include "yiche.h"

vector_t *vector_create(size_t element_size, int capacity)
{
  if (element_size == 0)
    exit_with_error("vector_create(): 'element_size' must be a positive integer");

  if (capacity <= 0)
    exit_with_error("vector_create(): 'capacity' must be a positive integer");

  vector_t *vector = malloc(sizeof(vector_t));
  if (vector == NULL)
    return NULL;

  vector->arr = malloc(element_size * capacity);
  if (vector->arr == NULL)
  {
    free(vector);
    return NULL;
  }

  vector->element_size = element_size;
  vector->length = 0;
  vector->capacity = capacity;

  return vector;
}

void vector_free(vector_t *vector)
{
  free(vector->arr);
  free(vector);
}

void *vector_next_element(vector_t *vector)
{
  if (vector->length == vector->capacity)
  {
    int new_capacity = vector->capacity * 2;

    void *new = realloc(vector->arr, vector->element_size * new_capacity);
    if (new == NULL)
      return NULL;

    vector->capacity = new_capacity;
    vector->arr = new;
  }

  return ((unsigned char*)vector->arr) + vector->element_size * vector->length++;
}

VECTOR_T(void*) *vector_pointer_create(int capacity)
{
  return vector_create(sizeof(void*), capacity);
}

int vector_pointer_append(VECTOR_T(void*) *vector, void *elt)
{
  void **p = vector_next_element(vector);
  if (p == NULL)
    return 0;

  *p = elt;
  return 1;
}
