#include "sqldb.h"
#include <stddef.h>
#include <stdio.h>

void tokenize_debug(Token *root_token) {
  for (; root_token->type != TOKEN_EOF; root_token = root_token->next) {
    printf("%-*s -> %s\n", 10, root_token->str, get_token_name(root_token));
  }
}

void parse_debug(Node *node, int cur_depth) {
  if (node->childs->cnt > 0) {
    for (size_t i = 0; i < node->childs->cnt; i++) {
      parse_debug(&node->childs->items[i], cur_depth + 1);
    }
  }
  printf("depth:%-*dNodename:%s\n", cur_depth + 5, cur_depth,
         get_node_name(node));
}

int main(int argc, char *argv[]) {
  Token *a = tokenize(argv[1]);
  tokenize_debug(a);

  Node *node = parse(a);
  parse_debug(node, 0);

  return 0;
}
