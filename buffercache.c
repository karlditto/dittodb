#include "sqldb.h"
#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>

void execute_sql(Node *node, BufferCache *buffer) {
  assert(node);
  printf("%s\n", node->token->str);
  for (size_t i = 0; i < node->childs->cnt; i++) {
    Node *cur_node = at(node->childs, i);
    execute_sql(cur_node, buffer);
  }
}
