#include "sqldb.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

Region r;

int main(int argc, char *argv[]) {

  char *command = NULL;
  size_t numread;
  while (true) {
    r = meta_alloc(10 * 1024 * 1024);
    printf("SQL> ");
    ssize_t num_bytes = getline(&command, &numread, stdin);
    if (num_bytes == -1) {
      printf("null byte received, exit process.\n");
      break;
    }
    Token *token = tokenize(command);

    print_token(token);

    Node *ast = parse(token);
    print_ast(ast, 1);
    // token_free(token);
    region_free(&r);
  }
  free(r.data);
}
