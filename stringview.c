#include "sqldb.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>

StringView sv_from_cstr(const char *cstr) {
  return (StringView){
      .cstr = cstr,
      .len = strlen(cstr),
  };
}

StringView sv_ltrim(StringView sv) {
  size_t i = 0;
  while (i < sv.len && isspace(*sv.cstr)) {
    i++;
  }
  return (StringView){
      .cstr = sv.cstr + i,
      .len = sv.len - i,
  };
}

StringView sv_rtrim(StringView sv) {
  size_t i = 0;
  while (i < sv.len && isspace(sv.cstr[sv.len - 1 - i])) {
    i++;
  }
  return (StringView){
      .cstr = sv.cstr,
      .len = sv.len - i,
  };
}

StringView sv_trim(StringView sv) { return sv_rtrim(sv_ltrim(sv)); }

StringView sv_cut_to_delim(StringView *sv, char delim) {
  size_t i = 0;
  while (i < sv->len && sv->cstr[i] != delim) {
    i++;
  }

  StringView result = {
      .cstr = sv->cstr,
      .len = i,
  };

  if (i < sv->len) {
    sv->len -= i + 1;
    sv->cstr += i + 1;
  } else { // handling overflow
    sv->len -= i;
    sv->cstr += i;
  }

  return result;
}

int sv_eq(StringView a, StringView b) {
  if (a.len != b.len) {
    return 0;
  } else {
    return memcmp(a.cstr, b.cstr, a.len) == 0;
  }
}
