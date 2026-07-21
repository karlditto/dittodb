#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
typedef struct {
  size_t capacity;
  size_t used;
  char *data;
} Region;

Region region_init(size_t capacity);
void *region_alloc(Region *r, size_t size);
void region_free(Region *r);
