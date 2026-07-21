#include "region.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

Region region_init(size_t capacity) {
  void *data = calloc(1, capacity);
  assert(data != NULL);

  Region r = {.capacity = capacity, .data = data};
  return r;
}

void *region_alloc(Region *r, size_t size) {
  if (r == NULL) {
    return calloc(1, size);
  }

  assert(r->used + size <= r->capacity);

  void *result = &r->data[r->used];
  r->used += size;
  return result;
}

void region_free(Region *r) {
  r->used = 0;
  free(r->data);
  r->capacity = 0;
}
