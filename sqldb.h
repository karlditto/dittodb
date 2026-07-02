#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
  uint64_t *items;
  size_t cnt;
  size_t capa;
} Array;

#define append(xs ,x)\
  do {\
    if (xs.cnt >= xs.capa) {\
      if (xs.capa == 0) xs.capa = 256;\
      else xs.capa *= 2;\
      xs.items = realloc(xs.items, xs.capa*sizeof(*xs.items));\
    }\
    xs.items[xs.cnt++] = x;\
  } while(0)

