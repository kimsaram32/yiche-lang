#include <string.h>
#include "yiche.h"

typedef struct hash_table_node_t
{
  char *key;
  void *payload;
  struct hash_table_node_t *next, *prev;
}
hash_table_node_t;

typedef struct hash_table_t
{
  size_t capacity, size;
  hash_table_node_t **buckets;
  payload_destructor_t payload_destructor;
}
hash_table_t;

// djb2
// http://www.cse.yorku.ca/~oz/hash.html
static size_t hash_table_compute_hash(char *key, size_t capacity)
{
  size_t h = 5381;

  while (*key)
    h = ((h << 5) + h) + *key++;

  return h % capacity;
}

static int is_prime(int n)
{
  if (n < 2)
    return 0;

  for (int i = 2; i * i <= n; i++)
    if (n % i == 0)
      return 0;

  return 1;
}

// n is a positive integer
// Finds a smallest prime number p such that p >= n.
static size_t next_prime(size_t n)
{
  while (!is_prime(n))
    n++;

  return n;
}

hash_table_node_t *hash_table_node_create(char *key, void *payload)
{
  hash_table_node_t *node = malloc(sizeof(hash_table_node_t));
  if (node == NULL)
    return NULL;

  node->key = malloc(sizeof(char) * (strlen(key) + 1));
  if (node->key == NULL)
  {
    free(node);
    return NULL;
  }
  strcpy(node->key, key);
  node->payload = payload;

  return node;
}

static void hash_table_node_free(hash_table_node_t *node, hash_table_t *table)
{
  free(node->key);
  table->payload_destructor(node->payload);
  free(node);
}

static hash_table_node_t **hash_table_buckets_array_create(size_t capacity)
{
  hash_table_node_t **buckets = malloc(sizeof(hash_table_node_t*) * capacity);

  if (buckets == NULL)
    return NULL;

  for (size_t i = 0; i < capacity; i++)
    buckets[i] = NULL;

  return buckets;
}

static int hash_table_resize(hash_table_t *table, size_t new_capacity)
{
  hash_table_node_t **new_buckets = hash_table_buckets_array_create(new_capacity);
  if (new_buckets == NULL)
    return 0;

  for (size_t i = 0; i < table->capacity; i++)
    for (hash_table_node_t *node = table->buckets[i], *next; node != NULL; node = next)
    {
      next = node->next;

      size_t hash = hash_table_compute_hash(node->key, new_capacity);
      node->prev = NULL;
      node->next = new_buckets[hash];
      if (new_buckets[hash] != NULL)
        new_buckets[hash]->prev = node;
      new_buckets[hash] = node;
    }

  table->capacity = new_capacity;
  free(table->buckets);
  table->buckets = new_buckets;

  return 1;
}

hash_table_t *hash_table_create(size_t capacity, payload_destructor_t payload_destructor)
{
  if (capacity == 0)
    exit_with_error("hash_table_create(): 'capacity' must be a positive integer");

  hash_table_t *table = malloc(sizeof(hash_table_t));
  if (table == NULL)
    return NULL;

  if ((table->buckets = hash_table_buckets_array_create(capacity)) == NULL)
  {
    free(table);
    return NULL;
  }

  table->capacity = capacity;
  table->size = 0;
  table->payload_destructor = payload_destructor;

  return table;
}

void hash_table_free(hash_table_t *table)
{
  for (size_t i = 0; i < table->capacity; i++)
    for (hash_table_node_t *node = table->buckets[i], *next; node != NULL; node = next)
    {
      next = node->next;
      hash_table_node_free(node, table);
    }

  free(table->buckets);
  free(table);
}

void *hash_table_search(hash_table_t *table, char *key)
{
  size_t hash = hash_table_compute_hash(key, table->capacity);

  hash_table_node_t *node = table->buckets[hash];
  while (node != NULL && strcmp(node->key, key))
    node = node->next;

  if (node == NULL)
    return NULL;

  return node->payload;
}

int hash_table_insert(hash_table_t *table, char *key, void *payload)
{
  hash_table_node_t *node = hash_table_node_create(key, payload);
  if (node == NULL)
    return 0;

  size_t hash = hash_table_compute_hash(key, table->capacity);
  node->prev = NULL;
  node->next = table->buckets[hash];
  if (table->buckets[hash] != NULL)
    table->buckets[hash]->prev = node;
  table->buckets[hash] = node;

  if (++table->size > table->capacity)
    hash_table_resize(table, next_prime(table->capacity * 2));

  return 1;
}

void hash_table_delete(hash_table_t *table, char *key)
{
  size_t hash = hash_table_compute_hash(key, table->capacity);

  hash_table_node_t *node = table->buckets[hash];
  while (node != NULL && strcmp(node->key, key))
    node = node->next;

  if (node == NULL)
    exit_with_error("hash_table_delete(): entry with key %s does not exist", key);

  if (node->prev == NULL)
    // first node
    table->buckets[hash] = node->next;
  else
    node->prev->next = node->next;

  if (node->next != NULL)
    node->next->prev = node->prev;

  hash_table_node_free(node, table);

  table->size--;
}
