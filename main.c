#include "sqldb.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARENA_SIZE_BYTES 20 * 1024 * 1024

Region mem_ast;

int main_parser(int argc, char *argv[]) {

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

int main_pagedir() {
  PageDirectory pd = {0};
  hash_init(&pd, HASH_BUCKET_SIZE);
  hash_append(&pd, "movie"); // case sensitive
  int hashidx = hash("movie", 5) % pd.capa;
  UsedPageAppend(&pd.items[hashidx].usedpageno, 2);
  UsedPageAppend(&pd.items[hashidx].usedpageno, 3);
  UsedPageAppend(&pd.items[hashidx].usedpageno, 4);
  hash_append(&pd, "director"); // case sensitive
  hashidx = hash("director", 8) % pd.capa;
  UsedPageAppend(&pd.items[hashidx].usedpageno, 9);
  UsedPageAppend(&pd.items[hashidx].usedpageno, 12);
  UsedPageAppend(&pd.items[hashidx].usedpageno, 16);

  pd_todisk(&pd, "pagedir.db");

  PageDirectory pd_new = pd_fromdisk("pagedir.db");
  pd_todisk(&pd_new, "pagedir.db2");

  for (size_t i = 0; i < pd.capa; i++) {
    free(pd.items[i].usedpageno.items);
  }
  free(pd.items);

  for (size_t i = 0; i < pd.capa; i++) {
    free(pd_new.items[i].usedpageno.items);
  }
  free(pd_new.items);

  return 0;
}

int main() {
  char *filename = "my.db";
  int fd = open(filename, O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0755);
  if (fd == -1) {
    fd = open(filename, O_RDWR | S_IRUSR | S_IWUSR | O_TRUNC, 0755);
    errno = 0;
  }
  assert(fd >= 0);

  PageDirectory pd = {0};
  hash_init(&pd, HASH_BUCKET_SIZE);

  RootPage rp = {0};
  rp.pagecnt++;
  rp.pageno = 0;

  Page page = {0};
  page.pageno = rp.pagecnt++;
  page.pagetype = TABLE_PAGE;
  page.freespace = sizeof(page.data);
  page.slotcnt = 0;

  hash_append(&pd, "DataDict"); // case sensitive
  int hashidx = hash("DataDict", 5) % pd.capa;
  UsedPageAppend(&pd.items[hashidx].usedpageno, page.pageno);
  pd_todisk(&pd, "pagedir.db");

  char schema[32] = "SYS";
  char table[128] = "DataDict";

  memcpy(page.data + page.freespace - sizeof(table) - sizeof(schema), table,
         sizeof(schema));
  memcpy(page.data + page.freespace - sizeof(table), table, sizeof(table));
  page.slotcnt++;
  page.freespace -= sizeof(schema) + sizeof(table);
  Slot slot = {
      .slotno = 1,
      .offset = sizeof(page.data) - 32 - 128,
  };

  ssize_t written = write(fd, &rp, sizeof(RootPage));
  assert(written >= 0);
  written = write(fd, &page, sizeof(Page));
  assert(written >= 0);

  close(fd);
  return 0;
}
