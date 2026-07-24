#include "storage.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// dynamic hash map implemented with open addressing
unsigned long hash(char *str) {
  // djb2
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

void kv_init(KV *kv) {
  assert(kv->capa == 0);
  if (kv->capa == 0) {
    kv->capa = 2;
  }
  kv->pageno = calloc(1, sizeof(*kv->pageno));
}

static int64_t hashmap_insert(HashMap *map, char *key) {
  assert(map->capa > map->cnt);
  unsigned long idx = hash(key) % map->capa;
  for (size_t i = 0; i < map->capa && map->items[idx].occupied &&
                     strcmp(map->items[idx].objname, key) != 0;
       i++) {

    if (map->items[idx].deleted) {
      if (hashmap_find(map, key) != -1) {
        printf("Keys collide\n");
        return -1;
      } else {
        strcpy(map->items[idx].objname, key);
        map->items[idx].deleted = false;
        return idx;
      }
    }
    idx = (idx + 1) % map->capa;
  }
  if (map->items[idx].occupied) {
    if (strcmp(key, map->items[idx].objname) != 0) {
      fprintf(stderr, "Hash map overflow\n");
      exit(EXIT_FAILURE);
    }
    printf("Keys collide\n");
  } else {
    strcpy(map->items[idx].objname, key);
    map->cnt++;
    map->items[idx].occupied = true;
    return idx;
  }
  return -1;
}

void hashmap_extend(HashMap *map, double factor) {
  assert(factor >= 1);
  size_t old_capa = map->capa;
  map->capa *= factor;
  // map->items = realloc(map->items, map->capa * sizeof(*map->items));
  KV *temp = map->items;
  size_t temp_cnt = map->cnt;
  map->items = calloc(1, map->capa * sizeof(*map->items));
  map->cnt = 0;
  for (size_t i = 0; i < old_capa; i++) {
    if (temp[i].deleted) {
      continue;
    }
    if (strlen(temp[i].objname) == 0) {
      continue;
    }
    hashmap_insert(map, temp[i].objname);
    int64_t idx = hashmap_find(map, temp[i].objname);
    map->items[idx].cnt = temp[i].cnt;
  }
  free(temp);
}

int64_t hashmap_find(HashMap *map, char *key) {
  unsigned long idx = hash(key) % map->capa;
  for (size_t i = 0; i < map->capa && map->items[idx].occupied &&
                     strcmp(map->items[idx].objname, key) != 0;
       i++) {
    idx = (idx + 1) % map->capa;
  }
  if (!map->items[idx].occupied) {
    // printf("Key not found\n");
    return -1;
  }
  if (map->items[idx].occupied && strcmp(map->items[idx].objname, key) == 0) {
    return idx;
  }
  return -1;
}

int64_t hashmap_append(HashMap *map, char *key) {
  assert(map->items != NULL);
  if (map->cnt >= map->capa) {
    if (map->capa == 0) {
      map->capa = 256;
    } else {
      hashmap_extend(map, 2);
    }
  }
  return hashmap_insert(map, key);
}

void hashmap_init(HashMap *map, size_t bucket) {
  // must init hashmap after declare it
  map->items = calloc(1, sizeof(KV) * bucket);
  map->capa = bucket;
}

void hashmap_free(HashMap *map) {
  for (size_t i = 0; i < map->capa; i++) {
    free(map->items[i].pageno);
  }
  free(map->items);
}

void hashmap_print(HashMap *map) {
  for (size_t i = 0; i < map->capa; i++) {
    unsigned long h = hash(map->items[i].objname);
    printf("hash: 0x%024lX | idx: %-3lu | key: %-10s | deleted: %d\n", h, i,
           map->items[i].objname, map->items[i].deleted);
  }
}

void hashmap_delete(HashMap *map, char *key) {
  int64_t idx = hashmap_find(map, key);
  if (idx < 0) {
    return;
  }
  map->items[idx].deleted = true;
  map->items[idx].capa = 0;
  map->items[idx].cnt = 0;
  memset(map->items[idx].objname, 0, strlen(map->items[idx].objname));
  free(map->items[idx].pageno);
}
