#include "storage.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void UsedPageAppend(UsedPageNO *arr, uint64_t pageno) {
  if (arr->cnt >= arr->capa) {
    if (arr->capa == 0) {
      arr->capa = 8;
    } else {
      arr->capa *= 2;
    }
    arr->items = realloc(arr->items, arr->capa * sizeof(*arr->items));
  }
  arr->items[arr->cnt++] = pageno;
}

uint64_t hash(char *s, size_t len) {
  uint64_t result = 0;
  for (size_t i = 0; i < len; i++) {
    result += result * 31 + (uint64_t)s[i];
  }
  return result;
}

void hash_append(PageDirectory *ht, char *key) {
  uint64_t h = hash(key, strlen(key)) % ht->capa;
  for (size_t i = 0; i < ht->capa && ht->items[h].occupied &&
                     strcmp(key, ht->items[h].objname) != 0;
       ++i) {
    h = (h + 1) % ht->capa;
  }
  if (ht->items[h].occupied) {
    if (strcmp(key, ht->items[h].objname) != 0) {
      fprintf(stderr, "Hash table overflow\n");
      exit(EXIT_FAILURE);
    }
    printf("This key has already been added\n");
  } else {
    strcpy(ht->items[h].objname, key);
    ht->items[h].occupied = true;
    ht->cnt++;
  }
}

uint64_t hash_find(PageDirectory *ht, char *key) {
  uint64_t h = hash(key, strlen(key)) % ht->capa;
  for (size_t i = 0; i < ht->capa && ht->items[h].occupied &&
                     !(strcmp(key, ht->items[h].objname) == 0);
       i++) {
    h = (h + 1) % ht->capa;
  }
  if (!ht->items[h].occupied) {
    return NULL;
  }
  if (ht->items[h].occupied && strcmp(ht->items[h].objname, key) == 0) {
    return h;
  }

  return NULL;
}

size_t pd_todisk(PageDirectory *pd, char *filename) {
  ToDiskBuf buf = {0};
  todiskbuf_append(buf, pd, sizeof(*pd) - sizeof(KV *));
  for (size_t i = 0; i < pd->capa; i++) {
    todiskbuf_append(buf, &pd->items[i], sizeof(KV) - sizeof(uint64_t *));
    todiskbuf_append(buf, pd->items[i].usedpageno.items,
                     pd->items[i].usedpageno.cnt * sizeof(uint64_t));
  }
  int fd = open(filename, O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0755);
  if (fd == -1) {
    fd = open(filename, O_RDWR | S_IRUSR | S_IWUSR | O_TRUNC, 0755);
    errno = 0;
  }

  assert(fd >= 0);

  ssize_t written = write(fd, buf.data, buf.used);

  assert(written >= 0);
  close(fd);

  free(buf.data);
  return written;
}

PageDirectory pd_fromdisk(char *filename) {
  struct stat statbuf;

  size_t capa = 0;
  PageDirectory pd = {0};

  int fd = open(filename, O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0755);
  if (fd == -1) {
    fd = open(filename, O_RDWR | S_IRUSR | S_IWUSR, 0755);
    errno = 0;
  }

  assert(fd >= 0);

  int res = fstat(fd, &statbuf);
  assert(res == 0);
  char data[statbuf.st_size];
  // char *data = calloc(1, statbuf.st_size);
  ssize_t readed = read(fd, data, statbuf.st_size);
  assert(readed >= 0);

  close(fd);

  char *dptr = data; // sliding pointer

  memcpy(&capa, dptr, sizeof(pd.capa));
  hash_init(&pd, capa);
  dptr = dptr + sizeof(pd.capa);
  dptr = dptr + sizeof(pd.cnt);
  for (size_t i = 0; i < pd.capa; i++) {
    char objname[OBJ_NAME_LEN];
    memcpy(objname, dptr, OBJ_NAME_LEN);
    if (strlen(objname) == 0) {
      dptr = dptr + sizeof(KV) - sizeof(uint64_t *);
      continue;
    }
    dptr = dptr + OBJ_NAME_LEN;
    hash_append(&pd, objname); // case sensitive
    int hashidx = hash(objname, strlen(objname)) % pd.capa;
    dptr = dptr + sizeof(KV) - OBJ_NAME_LEN - sizeof(UsedPageNO);
    dptr = dptr + sizeof(size_t);

    size_t arrcnt = 0;
    memcpy(&arrcnt, dptr, sizeof(size_t));
    dptr = dptr + sizeof(pd.items[hashidx].usedpageno.cnt);

    for (size_t j = 0; j < arrcnt; j++) {
      uint64_t pageno;
      memcpy(&pageno, dptr, sizeof(uint64_t));
      dptr = dptr + sizeof(uint64_t);
      UsedPageAppend(&pd.items[hashidx].usedpageno, pageno);
    }
  }
  return pd;
}
