#include "sqldb.h"
#include <stdlib.h>

Array *new_arr() {
  Array *arr = calloc(1, sizeof(Array));
  return arr;
}
