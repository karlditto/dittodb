#include "../storage.h"
#include <bits/time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *gen_key_from_idx(int64_t idx, HashMap *map) {
  char *key = calloc(1, 9);
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  srand(ts.tv_nsec);
  while (true) {
    for (size_t i = 0; i < 8; i++) {
      long ascii = rand() % 95 + 32;
      key[i] = ascii;
      if (ascii == 0) {
        break;
      }
    }
    if (hash(key) % map->capa == idx) {
      return key;
    }
  }
}

int main() {

  char *keys[16];
  HashMap map = {0};
  hashmap_init(&map, 8);
  for (size_t i = 5; i < 7; i++) {
    char *key = gen_key_from_idx(i, &map);
    keys[i] = key;
    printf("key: %s\n", key);
    hashmap_append(&map, key);
  }
  printf("capa:%zu, cnt:%zu\n", map.capa, map.cnt);
  hashmap_print(&map);
  hashmap_delete(&map, keys[5]);
  printf("capa:%zu, cnt:%zu\n", map.capa, map.cnt);
  hashmap_print(&map);
  hashmap_extend(&map, 1.5);
  printf("capa:%zu, cnt:%zu\n", map.capa, map.cnt);
  hashmap_print(&map);
  return 0;
}
