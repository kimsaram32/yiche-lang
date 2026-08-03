#ifndef YICHE_HASH_TABLE_H
#define YICHE_HASH_TABLE_H

typedef struct hash_table_t hash_table_t;

// The macro 'HASH_TABLE_T(type)' denotes a hash table whose payload is a
// pointer to 'type'.
//
// Example usage:
// HASH_TABLE_T(symbol_table_entry_t) *symbol_table = hash_table_create(...);
#define HASH_TABLE_T(type) hash_table_t

// 'capacity' must be a prime number for efficient hashing. 'payload_destructor'
// is called whenever a node gets freed in 'hash_table_delete' or
// 'hash_table_free', and it should deallocate the entire payload.
hash_table_t *hash_table_create(size_t capacity, destructor_t payload_destructor);

void hash_table_free(hash_table_t *table);

// Returns the pointer to the payload, or NULL if the entry is not found.
void *hash_table_search(hash_table_t *table, char *key);

// Insert an entry to 'table' with 'key' and 'payload'. Assumes that there is no
// entry in 'table' with the same key, and no further checks are performed.
//
// Returns 1 if the entry gets successfully inserted, and 0 if an out-of-memory
// error occurs.
int hash_table_insert(hash_table_t *table, char *key, void *payload);

// Throws an error if an entry with the key does not exist.
void hash_table_delete(hash_table_t *table, char *key);

#endif
