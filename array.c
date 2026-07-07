#include "sqldb.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

void append(Array *arr, Node *item) {
  if (arr->cnt >= arr->capa) {
    if (arr->capa == 0) {
      arr->capa = 32;
    } else {
      arr->capa *= 2;
    }
    arr->items = realloc(arr->items, arr->capa * sizeof(*arr->items));
  }
  arr->items[arr->cnt++] = *item;
}

Node *pop(Array *arr) { return &arr->items[--(arr->cnt)]; }

Node *first(Array *arr) { return &arr->items[assert(arr->cnt > 0), 0]; }

Node *last(Array *arr) {
  return &arr->items[assert(arr->cnt > 0), arr->cnt - 1];
}

Node *at(Array *arr, int index) {
  return &arr->items[assert(arr->cnt > index), index];
}
