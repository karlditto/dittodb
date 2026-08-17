#include "hashmap.h"
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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
        assert(strlen(key) < sizeof(map->items[idx].objname));
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
  KV *temp = calloc(1, sizeof(*map->items) * map->capa);
  memcpy(temp, map->items, sizeof(*map->items) * map->capa);
  free(map->items);
  map->items = NULL;
  size_t old_capa = map->capa;
  map->capa *= factor;
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
    map->items[idx].capa = temp[i].capa;
    map->items[idx].cnt = temp[i].cnt;
    map->items[idx].pageno = calloc(1, sizeof(*temp[i].pageno) * temp[i].capa);
    memcpy(map->items[idx].pageno, temp[i].pageno,
           temp[i].capa * sizeof(*temp[i].pageno));
    // for (size_t j = 0; j < temp[i].cnt; j++) {
    //   KV_append(map->items[idx], temp[i].pageno[j]);
    // }
    if (temp[i].pageno == NULL) {
      continue;
    }
    free(temp[i].pageno);
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
    if (map->items[i].pageno == NULL) {
      continue;
    }
    free(map->items[i].pageno);
  }
  free(map->items);
  map->capa = 0;
  map->cnt = 0;
}

void hashmap_print(HashMap *map) {
  for (size_t i = 0; i < map->capa; i++) {
    unsigned long h = hash(map->items[i].objname);
    printf("hash: 0x%024lX | idx: %-3lu | key: %-10s | deleted: %d\n", h, i,
           map->items[i].objname, map->items[i].deleted);
    printf("Pageno: {");
    for (size_t j = 0; j < map->items[i].cnt; j++) {
      printf("%ld,", map->items[i].pageno[j]);
    }
    printf("}\n");
  }
}

void hashmap_delete(HashMap *map, char *key) {
  int64_t idx = hashmap_find(map, key);
  if (idx < 0) {
    // printf("The key you want to delete doesnt exist\n");
    return;
  }
  map->items[idx].deleted = true;
  map->items[idx].capa = 0;
  map->items[idx].cnt = 0;
  memset(map->items[idx].objname, 0, strlen(map->items[idx].objname));
  free(map->items[idx].pageno);
  map->items[idx].pageno = NULL;
}

void hashmap_todisk(HashMap *map, char *filename) {
  int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  assert(fd >= 0);
  ssize_t written = write(fd, map, sizeof(*map) - sizeof(map->items));
  assert(written >= 0);
  for (size_t i = 0; i < map->capa; i++) {
    written = write(fd, &map->items[i],
                    sizeof(map->items[i]) - sizeof(map->items[i].pageno));
    assert(written >= 0);
    written = write(
        fd, map->items[i].pageno,
        map->items[i].cnt *
            sizeof(*map->items[i].pageno)); // write only used part to memory to
                                            // minimize disk usage
  }
}

void hashmap_fromdisk(HashMap *map, char *filename) {
  int fd = open(filename, O_RDONLY);
  assert(fd > 0);
  ssize_t readed = read(fd, map, sizeof(*map) - sizeof(map->items));
  assert(readed >= 0);
  map->items = calloc(1, sizeof(KV) * map->capa);
  assert(map->items);
  for (size_t i = 0; i < map->capa; i++) {
    readed = read(fd, &map->items[i], sizeof(KV) - sizeof(uint64_t *));
    assert(readed >= 0);
    map->items[i].pageno =
        map->items[i].capa ? calloc(1, sizeof(uint64_t) * map->items[i].capa)
                           : NULL;
    readed =
        read(fd, map->items[i].pageno, sizeof(uint64_t) * map->items[i].cnt);
    assert(readed >= 0);
  }
}
