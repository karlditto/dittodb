#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#define DB_PAGE_SIZE 4 * 1024 // in bytes
#define OBJ_NAME_LEN 128
#define HASH_BUCKET_SIZE 1024 * 53

// Bit array
#define CHAR_BIT 8
#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

// page directory
typedef struct {
  size_t capa;
  size_t cnt;
  uint64_t *items;
} UsedPageNO;

typedef struct {
  char objname[OBJ_NAME_LEN];
  bool occupied;
  UsedPageNO usedpageno;
} KV;

typedef struct {
  size_t capa;
  size_t cnt;
  KV *items;
} PageDirectory;

typedef struct {
  size_t capa;
  size_t used;
  char *data;
} ToDiskBuf;

#define hash_init(ht, cap)                                                     \
  do {                                                                         \
    (ht)->items = calloc(1, sizeof(*(ht)->items) * cap);                       \
    (ht)->cnt = 0;                                                             \
    (ht)->capa = cap;                                                          \
  } while (0)

void UsedPageAppend(UsedPageNO *arr, uint64_t pageno);
uint64_t hash(char *s, size_t len);
void hash_append(PageDirectory *ht, char *key);

#define todiskbuf_append(buf, ptr, size)                                       \
  do {                                                                         \
    if (buf.used + size >= buf.capa) {                                         \
      if (buf.capa == 0) {                                                     \
        buf.capa = 256;                                                        \
      } else {                                                                 \
        buf.capa *= 2;                                                         \
      }                                                                        \
      buf.data = realloc(buf.data, buf.capa);                                  \
    }                                                                          \
    memcpy(buf.data + buf.used, ptr, size);                                    \
    buf.used += size;                                                          \
  } while (0)

size_t pd_todisk(PageDirectory *pd, char *filename);
PageDirectory pd_fromdisk(char *filename);

typedef enum {
  TABLE_PAGE,
  INDEX_PAGE,
  ROOT_PAGE,
  PAGETYPECNT,
} PageType;

typedef struct {
  unsigned long long pageno;
  unsigned long long pagecnt;
  PageType pagetype;

  char data[DB_PAGE_SIZE - sizeof(unsigned long long) * 3];
} RootPage;

// data page
typedef struct {
  // header
  unsigned long long pageno; // start from 0;
  unsigned long long slotcnt;
  unsigned short freespace; // in bytes
  PageType pagetype;

  // data section
  char data[DB_PAGE_SIZE - sizeof(unsigned long long) * 3];
} Page;

typedef struct {
  unsigned short slotno;
  unsigned short offset;
} Slot;
