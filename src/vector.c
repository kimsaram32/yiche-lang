#include <stdlib.h>
#include "string.h"
#include "yiche.h"

static void *vector_elt_p(vector_t *vector, int i)
{
  return ((unsigned char*)vector->arr) + vector->element_size * i;
}

vector_t *vector_create(size_t element_size, int capacity, destructor_t destructor)
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
  vector->destructor = destructor;

  return vector;
}

void vector_free(vector_t *vector)
{
  for (int i = 0; i < vector->length; i++)
    vector->destructor(vector_elt_p(vector, i));

  free(vector->arr);
  free(vector);
}

int vector_append(vector_t *vector, void *elt)
{
  if (vector->length == vector->capacity)
  {
    int new_capacity = vector->capacity * 2;

    void *new = realloc(vector->arr, vector->element_size * new_capacity);
    if (new == NULL)
      return 0;

    vector->capacity = new_capacity;
    vector->arr = new;
  }

  void *p = vector_elt_p(vector, vector->length++);
  memcpy(p, elt, vector->element_size);

  return 1;
}
