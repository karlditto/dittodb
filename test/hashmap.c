#include "../hashmap.h"
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

char *gen_key() {
  char *key = calloc(1, 9);
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  srand(ts.tv_nsec);
  for (size_t i = 0; i < 8; i++) {
    long ascii = rand() % 95 + 32;
    key[i] = ascii;
    if (ascii == 0) {
      break;
    }
  }
  return key;
}

void Elapsed_time(struct timespec begin, struct timespec end) {
  double s = (double)begin.tv_sec + begin.tv_nsec * 1e-9;
  double e = (double)end.tv_sec + end.tv_nsec * 1e-9;
  printf("Elapsed time: %lfs\n", e - s);
}

static int test_large_data() {

  char *keys[512];
  HashMap map = {0};
  hashmap_init(&map, 8);
  for (size_t i = 0; i < 256; i++) {
    char *key = gen_key();
    hashmap_append(&map, key);
    keys[i] = key;
  }
  printf("capa:%zu, cnt:%zu\n", map.capa, map.cnt);
  for (size_t j = 0; j < 4096; j++) {
    for (size_t i = 0; i < 128; i++) {

      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      srand(ts.tv_nsec);
      hashmap_delete(&map, keys[rand() % 128]);
    }
    for (size_t i = 0; i < 512; i++) {
      char *key = gen_key();
      int idx = hashmap_append(&map, key);
      if (idx == -1) {
        // printf("key: %s exists at idx %ld\n", key, hashmap_find(&map, key));
      }
      keys[i] = key;
    }
  }
  hashmap_extend(&map, 1);
  printf("capa:%zu, cnt:%zu\n", map.capa, map.cnt);
  // hashmap_print(&map);
  return 0;
}

static int test_performance() {
  HashMap pagedir = {0};
  hashmap_init(&pagedir, 256);
  char searchkey[32];

  struct timespec begin;
  struct timespec end;
  printf("=== Insert 4tb worth of page ===\n");
  clock_gettime(CLOCK_MONOTONIC, &begin);
  for (size_t i = 0; i < 52255; i++) {
    char *key = gen_key();
    hashmap_append(&pagedir, key);
    int idx = hashmap_find(&pagedir, key);
    memcpy(searchkey, key, strlen(key));
    free(key);
    for (size_t j = 0; j < 30250; j++) {
      KV_append(pagedir.items[idx], j);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  Elapsed_time(begin, end);
  printf("capa:%zu, cnt:%zu\n", pagedir.capa, pagedir.cnt);
  printf("=== start extend ===\n");
  clock_gettime(CLOCK_MONOTONIC, &begin);
  hashmap_extend(&pagedir, 2);
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("=== finish extend ===\n");
  Elapsed_time(begin, end);

  printf("%s\n", searchkey);
  printf("=== start search ===\n");
  clock_gettime(CLOCK_MONOTONIC, &begin);
  int idx = hashmap_find(&pagedir, searchkey);
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("=== end search ===\n");
  Elapsed_time(begin, end);
  if (idx == -1) {
    printf("key:%s not found\n", searchkey);
  } else {
    printf("%zu, %zu\n", pagedir.items[idx].capa, pagedir.items[idx].capa);
  }
  hashmap_free(&pagedir);

  return 0;
}

int main() {
  puts("hello world");
  return 0;
}
