#include "sqldb.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void mem_append(MemArea *mem, DB_Page page) {
  int memloc = -1;
  if (mem->freelist_head != NULL) {
    // reuse buffer in free list
    mem->buffer[mem->freelist_head->pageno] = page;
    memloc = mem->freelist_head->pageno;
    mem->freelist_head = mem->freelist_head->next;
  } else {
    if (mem->used >= mem->size) {
      if (mem->size == 0)
        mem->size = 64;
      else
        mem->size <<= 1;
      mem->buffer = realloc(mem->buffer, sizeof(DB_Page) * mem->size);
    }
    memloc = mem->used;
    mem->buffer[mem->used++] = page;
  }
  char pageno_str[32];
  snprintf(pageno_str, OBJ_NAME_LEN, "%u", page.pageheader.pageno);
  int idx = hashmap_append(&mem->pagetable, pageno_str);
  assert(idx >= 0 && "Hash map failed to append value");
  snprintf(mem->pagetable.items[idx].objname, OBJ_NAME_LEN, "%s", pageno_str);
  KV_append(mem->pagetable.items[idx], memloc);
}
