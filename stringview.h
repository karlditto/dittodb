#include <stddef.h>
#include <string.h>
typedef struct {
  const char *cstr;
  size_t len;
} StringView;

StringView sv_from_cstr(const char *cstr);
StringView sv_ltrim(StringView sv);
StringView sv_rtrim(StringView sv);
StringView sv_trim(StringView sv);
StringView sv_cut_by_delim(StringView *sv,
                           char delim); // cut string from start, return string
                                        // that is cut, delim is discarded
int sv_eq(StringView a, StringView b);
