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

int main() {
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
  while (*s) {
    // if (ht.cnt >= 5) {
    //   break;
    // }

    if (isspace(*s) != 0 || *s == ',' || *s == '.' || *s == ';' || *s == ':' ||
        *s == '?' || *s == '!') {
      s++;
      continue;
    } else {
      char *s2 = s;
      while (true) {
        if (isspace(*s) != 0) {
          int len = s - s2;
          sprintf(word_buf, "%.*s", len, s2);
          // printf("%s\n", word_buf);
          // printf("%lu ", word_cnt);
          // printf("0x%08lX: ", hash(word_buf, strlen(word_buf)) % hashbucket);
          // printf("%lu: ", hash(word_buf, strlen(word_buf)) % hashbucket);
          // printf("%.*s\n", len, s2);
          if (hashmap_find(&ht, word_buf)) {
            word_cnt++;
            break;
          }
          hashmap_append(&ht, word_buf);
          word_cnt++;
          break;
        }
        s++;
      }
      continue;
    }
  }

  // hashmap_print(&ht);
  // hashmap_print(&ht);

  int cnt = 0;
  for (size_t i = 0; i < ht.capa; i++) {
    if (strlen(ht.items[i].objname) != 0) {
      cnt++;
    }
  }
  printf("%d\n", cnt);

  hashmap_free(&ht);
  return 0;
}
