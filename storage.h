#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#define DB_PAGE_SIZE 4 * 1024 // in bytes
#define OBJ_NAME_LEN 128
#define HASH_BUCKET_SIZE 1024 * 53

// Bit array
#define CHAR_BIT 8
#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

// Hash map

typedef struct {
  char objname[OBJ_NAME_LEN];
  bool occupied;
  bool deleted;
  size_t capa;
  size_t cnt;
  uint64_t *pageno;
} KV;

typedef struct {
  size_t capa;
  size_t cnt;
  KV *items;
} HashMap;

unsigned long hash(char *str);
void hashmap_extend(HashMap *map, double factor);
uint64_t hashmap_append(HashMap *map, char *key);
uint64_t hashmap_find(HashMap *map, char *key);
void hashmap_init(HashMap *map, size_t bucket);
void hashmap_free(HashMap *map);
void hashmap_print(HashMap *map);
void hashmap_delete(HashMap *map, char *key);
