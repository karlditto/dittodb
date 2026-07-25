#include "sqldb.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

int main() {
  HashMap pagedir = {0};
  hashmap_init(&pagedir, 8);

  hashmap_append(&pagedir, "DBA_TABLES");
  int idx = hashmap_find(&pagedir, "DBA_TABLES");
  KV_append(pagedir.items[idx], 2411);
  KV_append(pagedir.items[idx], 343);
  KV_append(pagedir.items[idx], 347);
  KV_append(pagedir.items[idx], 339);

  hashmap_append(&pagedir, "DBA_USERS");
  idx = hashmap_find(&pagedir, "DBA_USERS");
  KV_append(pagedir.items[idx], 2365);
  KV_append(pagedir.items[idx], 380);
  KV_append(pagedir.items[idx], 11);

  hashmap_delete(&pagedir, "DBA_TABLES");

  hashmap_append(&pagedir, "DBA_TABLES");
  idx = hashmap_find(&pagedir, "DBA_TABLES");
  KV_append(pagedir.items[idx], 2411);
  KV_append(pagedir.items[idx], 739);
  KV_append(pagedir.items[idx], 692);
  KV_append(pagedir.items[idx], 359);
  KV_append(pagedir.items[idx], 69);

  hashmap_extend(&pagedir, 2);
  hashmap_print(&pagedir);
  hashmap_free(&pagedir);
  return 0;
}
