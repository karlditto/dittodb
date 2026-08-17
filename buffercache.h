#include "hashmap.h"
#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#define DB_PAGE_SIZE 4096 // in bytes

typedef enum {
  INTEGER,
  FLOAT,
  CHAR,
  BOOL,
  DB_DATATYPE_CNT,
} DBDataType;

typedef enum {
  DBHEADER,
  PAGEDIR,
  DATADICT,
  TABLEPAGE,
  INDEXPAGE,
  PAGETYPECNT
} PageType;

typedef struct {
  // header section
  int64_t NextPage; // next page number;
  uint32_t PageNo;  // start from 0
  uint16_t FreeSpace;
  uint16_t SlotNum;
  uint16_t SlotBoundry; // start from the start of data[]
  uint16_t DataBoundry; // start from the end of data[]
  PageType PageType;
  uint16_t RecordNum;
  uint32_t Reserved;
  char ObjName[OBJ_NAME_LEN];

  // data section
  char data[DB_PAGE_SIZE - 32 - OBJ_NAME_LEN];
} Page;

typedef struct {
  uint16_t SlotNo; // start from 0
  uint16_t DataLoc;
} Slot;

typedef struct {
  char DBName[OBJ_NAME_LEN];
  uint64_t PageCount;
  int64_t FreeListHeader;
} DBHeader;

// data dict
typedef struct {
  char ObjName[OBJ_NAME_LEN];
  char ColName[OBJ_NAME_LEN];
  uint16_t Ord;
  DBDataType DataType;
  uint16_t DataTypeLen;
} TableDef;

typedef struct {
  char ObjName[OBJ_NAME_LEN];
  uint32_t PageNo; // start from 0 and represent starting page of a object
} PageDir;

#define init_page(page, pagetype, objname, pageno)                             \
  do {                                                                         \
    page.NextPage = -1;                                                        \
    page.PageNo = pageno;                                                      \
    page.FreeSpace = sizeof(page.data);                                        \
    page.SlotNum = 0;                                                          \
    page.SlotBoundry = 0;                                                      \
    page.DataBoundry = sizeof(page.data);                                      \
    page.PageType = pagetype;                                                  \
    strcpy(page.ObjName, objname);                                             \
  } while (0)

#define page_insert_row(page, row, rowsize)                                    \
  do {                                                                         \
    assert(sizeof(Slot) + rowsize <= (page).FreeSpace);                        \
    Slot slot = {0};                                                           \
    slot.SlotNo = (page).SlotNum++;                                            \
    (page).DataBoundry -= rowsize;                                             \
    slot.DataLoc = (page).DataBoundry;                                         \
    memcpy(&(page).data[(page).SlotBoundry], &slot, sizeof(slot));             \
    (page).SlotBoundry += sizeof((slot));                                      \
    memcpy(&(page).data[(page).DataBoundry], &(row), rowsize);                 \
    (page).FreeSpace -= rowsize + sizeof(slot);                                \
    (page).RecordNum++;                                                        \
  } while (0)

typedef struct {
  size_t capaacity;
  size_t used;
  Page *buffer;
  HashMap pagetable;
  HashMap pagedir;
} BufferCache;

#define last_page(cache)                                                       \
  ((cache).buffer[assert((cache).used > 0), (cache).used - 1])

#define header_page(cache) ((cache).buffer[assert((cache).used > 0), 0])

#define buffer_flush(cache)                                                    \
  do {                                                                         \
    (cache).used = 0;                                                          \
    memset((cache).buffer, 0, sizeof(*(cache).buffer) * (cache).capaacity);    \
    (cache).pagetable.cnt = 0;                                                 \
    hashmap_free(&(cache).pagetable);                                          \
    hashmap_init(&(cache).pagetable, 4);                                       \
  } while (0);

#define buffer_append(cache, page)                                             \
  do {                                                                         \
    if ((cache).used >= (cache).capaacity) {                                   \
      if ((cache).capaacity == 0) {                                            \
        (cache).capaacity = 8;                                                 \
      } else                                                                   \
        (cache).capaacity *= 2;                                                \
      (cache).buffer =                                                         \
          realloc((cache).buffer, (cache).capaacity * sizeof((page)));         \
    }                                                                          \
    memcpy(&(cache).buffer[(cache).used++], &(page), sizeof((page)));          \
  } while (0);

#define page_alloc(cache, pagetype, objname, vpageno)                          \
  do {                                                                         \
    Page page = {0};                                                           \
    init_page((page), (pagetype), (objname), (vpageno));                       \
    int buffer_idx = (cache).used;                                             \
    buffer_append((cache), page);                                              \
    char pageno[OBJ_NAME_LEN] = {0};                                           \
    int printed = snprintf(pageno, OBJ_NAME_LEN, "%lu", vpageno);              \
    assert(printed >= 0);                                                      \
    int hash_idx =                                                             \
        hashmap_append(&(cache).pagetable, pageno) % (cache).pagetable.capa;   \
    KV_append((cache).pagetable.items[hash_idx], buffer_idx);                  \
  } while (0)

#define checkpoint(cache, filename)                                            \
  do {                                                                         \
    FILE *file = fopen(filename, "r+");                                        \
    assert(file);                                                              \
    for (size_t i = 0; i < cache.used; i++) {                                  \
      int offset =                                                             \
          fseek(file, cache.buffer[i].PageNo * DB_PAGE_SIZE, SEEK_SET);        \
      assert(offset >= 0);                                                     \
      size_t written = fwrite(&cache.buffer[i], DB_PAGE_SIZE, 1, file);        \
      assert(written > 0);                                                     \
    }                                                                          \
    fclose(file);                                                              \
    buffer_flush(cache);                                                       \
  } while (0);

#define select_tabdef(mpage, TABLE)                                            \
  do {                                                                         \
    printf("%-18s %-18s %-18s %-18s %-18s\n", "ObjName", "ColName", "Ord",     \
           "DataTypeEnum", "DataTypeLen");                                     \
    for (size_t i = 0; i < (mpage).SlotNum; i++) {                             \
      Slot *slot = (Slot *)&(mpage).data[i * sizeof(Slot)];                    \
      TABLE *row = (TABLE *)&(mpage).data[slot->DataLoc];                      \
      printf("%-18s %-18s %-18d %-18d %-18d\n", row->ObjName, row->ColName,    \
             row->Ord, row->DataType, row->DataTypeLen);                       \
    }                                                                          \
  } while (0)

#define select_pagedir(mpage, TABLE)                                           \
  do {                                                                         \
    printf("%-18s %-18s\n", "ObjName", "PageNo");                              \
    for (size_t i = 0; i < (mpage).SlotNum; i++) {                             \
      Slot *slot = (Slot *)&(mpage).data[i * sizeof(Slot)];                    \
      TABLE *row = (TABLE *)&(mpage).data[slot->DataLoc];                      \
      printf("%-18s %-18d\n", row->ObjName, row->PageNo);                      \
    }                                                                          \
  } while (0)

#define insert_tabledef(buffer, mon, mcn, mo, mdt, mdtl)                       \
  do {                                                                         \
    TableDef row = {0};                                                        \
    strcpy(row.ObjName, (mon));                                                \
    strcpy(row.ColName, (mcn));                                                \
    row.Ord = (mo);                                                            \
    row.DataType = (mdt);                                                      \
    row.DataTypeLen = (mdtl);                                                  \
    page_insert_row(last_page((buffer)), row, sizeof(row));                    \
  } while (0);
