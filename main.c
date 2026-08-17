#define _GNU_SOURCE
#include "sqldb.h"
#include <assert.h>
#include <fcntl.h>
#include <iso646.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define ARENA_SIZE_BYTES 20 * 1024 * 1024

Region mem_ast;

#ifdef SQLPARSER

int main(int argc, char *argv[]) {

  if (argc > 2) {
    fprintf(stderr, "too many arguments.\n");
    exit(EXIT_FAILURE);
  }

  if (argc == 2) {
    mem_ast = region_init(ARENA_SIZE_BYTES);
    Token *token = tokenize(argv[1]);
    print_token(token);

    Node *ast = parse(token);
    print_ast(ast, 1);
    // token_free(token);
    region_free(&mem_ast);
    return 0;
  }

  char *command = NULL;
  size_t numread;
  while (true) {
    mem_ast = region_init(ARENA_SIZE_BYTES);
    printf("SQL> ");
    ssize_t num_bytes = getline(&command, &numread, stdin);
    if (num_bytes == -1) {
      printf("EOF received, exit process.\n");
      break;
    }
    Token *token = tokenize(command);

    print_token(token);

    Node *ast = parse(token);
    print_ast(ast, 1);
    // token_free(token);
    region_free(&mem_ast);
  }
  free(mem_ast.data);
  free(command);

  return 0;
}

#endif /* ifdef SQLPARSER */

int cmp(const void *a, const void *b) {
  const KV *akv = a;
  const KV *bkv = b;
  return bkv->cnt - akv->cnt;
}

int main_hashtable() {
  HashMap ht = {0};
  hashmap_init(&ht, 2);

  int fd = open("t8.shakespeare.txt", O_RDONLY);
  assert(fd > 0);
  struct stat *statbuf = malloc(sizeof(struct stat));
  assert(fstat(fd, statbuf) == 0);
  char buf[statbuf->st_size + 1];
  read(fd, buf, statbuf->st_size);
  close(fd);

  // read word by word from txt file
  char *s = buf;
  unsigned long word_cnt = 0;
  char word_buf[33] = {0};
  int flag = 0;
  while (*s) {
    // if (ht.cnt >= 16) {
    //   break;
    // }

    if (isspace(*s) != 0 || *s == ',' || *s == '.' || *s == ';' || *s == ':' ||
        *s == '?' || *s == '!') {
      s++;
      continue;
    } else {
      char *s2 = s;
      while (true) {
        if (isspace(*s) != 0 || *s == ',' || *s == '.' || *s == ';' ||
            *s == ':' || *s == '?' || *s == '!') {
          int len = s - s2;
          memset(word_buf, 0, sizeof(word_buf));
          sprintf(word_buf, "%.*s", len, s2);
          // printf("%s\n", word_buf);
          // printf("%lu ", word_cnt);
          // printf("0x%08lX: ", hash(word_buf, strlen(word_buf)) % hashbucket);
          // printf("%lu: ", hash(word_buf, strlen(word_buf)) % hashbucket);
          // printf("%.*s\n", len, s2);

          if (hashmap_find(&ht, word_buf) != -1) {
            int64_t found = hashmap_find(&ht, word_buf);
            ht.items[found].cnt++;
            word_cnt++;
            break;
          }

          hashmap_append(&ht, word_buf);
          int64_t insert = hashmap_find(&ht, word_buf);
          ht.items[insert].cnt++;
          word_cnt++;
          if (flag == 0) {
            if (hashmap_find(&ht, "the") != -1) {
              hashmap_delete(&ht, "the");
              flag = 1;
            }
          }
          break;
        }
        s++;
      }
      continue;
    }
  }

  // hashmap_print(&ht);
  //
  KV *sorted = malloc(sizeof(KV) * ht.capa);
  memcpy(sorted, ht.items, sizeof(KV) * ht.capa);
  qsort(sorted, ht.capa, sizeof(KV), cmp);

  for (size_t i = 0; i < ht.capa; i++) {
    printf("key:%-20s --> cnt:%ld\n", sorted[i].objname, sorted[i].cnt);
  }

  free(sorted);
  hashmap_free(&ht);

  return 0;
}

#include "buffercache.h"
#include "pokemon.h"

int main1() {
  BufferCache buffer = {0};
  hashmap_init(&buffer.pagedir, 8);
  hashmap_init(&buffer.pagetable, 4);

  page_alloc(buffer, DBHEADER, "Database Header", (unsigned long)0);
  DBHeader dbheader = {
      .DBName = "DittoDB", .PageCount = 1, .FreeListHeader = -1};

  page_insert_row(last_page(buffer), dbheader, sizeof(DBHeader));

  // to disk;
  int fd = open("real.db", O_CREAT | O_TRUNC | O_RDWR | O_SYNC | O_DIRECT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  assert(fd >= 0);
  ssize_t written = write(fd, buffer.buffer, buffer.used * DB_PAGE_SIZE);
  assert(written >= 0);
  close(fd);

  buffer_flush(buffer);

  fd = open("real.db", O_RDWR | O_SYNC | O_DIRECT,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  assert(fd >= 0);

  Page *headerpage =
      mmap(NULL, sizeof(Page), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  Slot *slot = (Slot *)headerpage->data;
  DBHeader *header = (DBHeader *)&headerpage->data[slot->DataLoc];

  hashmap_append(&buffer.pagedir, "PAGE_DIRECTORY");
  int idx = hashmap_find(&buffer.pagedir, "PAGE_DIRECTORY");
  KV_append(buffer.pagedir.items[idx], header->PageCount);
  page_alloc(buffer, PAGEDIR, "PAGE_DIRECTORY", header->PageCount);
  header->PageCount++; // use auto increment outside of page_alloc because its a
                       // macro, ++ may be carried out multiple times

  hashmap_append(&buffer.pagedir, "TABLE_DEF");
  idx = hashmap_find(&buffer.pagedir, "TABLE_DEF");
  KV_append(buffer.pagedir.items[idx], header->PageCount);
  page_alloc(buffer, TABLEPAGE, "TABLE_DEF", header->PageCount);
  header->PageCount++;

  insert_tabledef(buffer, "TABLE_DEF", "ObjName", 0, CHAR, 32);
  insert_tabledef(buffer, "TABLE_DEF", "ColName", 1, CHAR, 32);
  insert_tabledef(buffer, "TABLE_DEF", "Ord", 2, INTEGER, 2);
  insert_tabledef(buffer, "TABLE_DEF", "DataType", 3, INTEGER, 4);
  insert_tabledef(buffer, "TABLE_DEF", "DataTypeLen", 4, INTEGER, 2);

  insert_tabledef(buffer, "PAGE_DIRECTORY", "ObjName", 0, CHAR, 32);
  insert_tabledef(buffer, "PAGE_DIRECTORY", "PageNo", 1, INTEGER, 4);

  int id = hashmap_find(&buffer.pagedir, "TABLE_DEF");
  int pageno = buffer.pagedir.items[id].pageno[0];
  char cpageno[32];
  sprintf(cpageno, "%d", pageno);
  int index = hashmap_find(&buffer.pagetable, cpageno);
  int pn = buffer.pagetable.items[index].pageno[0];
  select_tabdef(buffer.buffer[pn], TableDef);
  puts("");

  for (size_t i = 0; i < buffer.pagedir.capa; i++) {
    int id = hashmap_find(&buffer.pagedir, "PAGE_DIRECTORY");
    int pageno = buffer.pagedir.items[id].pageno[0];
    char cpageno[32];
    int index = hashmap_find(&buffer.pagetable, cpageno);
    int pn = buffer.pagetable.items[index].pageno[0];
    sprintf(cpageno, "%d", pageno);
    if (strlen(buffer.pagedir.items[i].objname) != 0) {
      PageDir row = {0};
      strcpy(row.ObjName, buffer.pagedir.items[i].objname);
      for (size_t j = 0; j < buffer.pagedir.items[i].cnt; j++) {
        row.PageNo = buffer.pagedir.items[i].pageno[j];
        page_insert_row(buffer.buffer[pn], row, sizeof(row));
      }
    }
  }

  id = hashmap_find(&buffer.pagedir, "PAGE_DIRECTORY");
  pageno = buffer.pagedir.items[id].pageno[0];
  memset(cpageno, 0, 32);
  sprintf(cpageno, "%d", pageno);
  index = hashmap_find(&buffer.pagetable, cpageno);
  pn = buffer.pagetable.items[index].pageno[0];
  select_pagedir(buffer.buffer[pn], PageDir);

  checkpoint(buffer, "real.db");
  close(fd);
  hashmap_free(&buffer.pagetable);
  free(buffer.buffer);
  return 0;
}
