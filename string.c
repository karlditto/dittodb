#include "sqldb.h"

StringView sv(const char *cstr) {
  return (StringView){
      .data = cstr,
      .len = strlen(cstr),
  }; // designated initializer syntax
}
