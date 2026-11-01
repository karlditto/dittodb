#include "hashmap.h"
#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#define DB_PAGE_SIZE                                                           \
  4096 // in bytes, cannot less than 512 bytes because O_DIRECT and disk spec

typedef struct {
  char objname[32];
  uint32_t pageno; // start from 0;
  uint32_t slotno;
  uint32_t freespace;
  uint32_t freespacetr; // start from the end of the data section
  int nextpage;
} DB_PageHeader;

typedef struct {
  DB_PageHeader pageheader;
  char data[DB_PAGE_SIZE - sizeof(DB_PageHeader)];
} DB_Page;

typedef struct {
  uint64_t pagecnt; // including dbheader
  int64_t freelist; // starting point of freelist linked list
} DatabaseHeader;

typedef struct {
  char objname[32];
  uint64_t index;
  char colname[32];
  char type[32];
  size_t len;
} TableDefinition;

typedef struct {
  char objname[32];
  uint64_t pageno;
} PageDirectory;

typedef struct FreeMem FreeMem;

struct FreeMem {
  int pageno; // pageno in memory
  FreeMem *next;
};

typedef struct {
  DB_Page *buffer;
  size_t used;
  size_t size;
  HashMap pagetable; // map of pageno to buffer location;
  HashMap pagedir;   // map of object to starting pageno on disk
  FreeMem *freelist_head;
  FreeMem *freelist_tail;
} MemArea;

#define init_mem(mem)                                                          \
  do {                                                                         \
    hashmap_init(&mem.pagetable, 16);                                          \
    hashmap_init(&mem.pagedir, 16);                                            \
  } while (0);

void mem_append(MemArea *mem, DB_Page page);
void init_page(DB_Page *page, char *objname);
