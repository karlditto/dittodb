#include "sqldb.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ucontext.h>

Node *new_node(NodeName name, NodeClass class, Array *arr) {
  Node *nnode = calloc(1, sizeof(Node));
  nnode->name = name;
  nnode->nodeclass = class;
  nnode->childs = arr;
  return nnode;
}

char *get_node_name(Node *node) {
  switch (node->name) {
  case NODE_CREATE:
    return "NODE_CREATE";
  case NODE_SELECT:
    return "NODE_SELECT";
  case NODE_INSERT:
    return "NODE_INSERT";
  case NODE_DELETE:
    return "NODE_DELETE";
  case NODE_FROM:
    return "NODE_FROM";
  case NODE_TABLE:
    return "NODE_TABLE";
  case NODE_ADD:
    return "NODE_ADD";
  case NODE_SUB:
    return "NODE_SUB";
  case NODE_MUL:
    return "NODE_MUL";
  case NODE_DIV:
    return "NODE_DIV";
  case NODE_NEG:
    return "NODE_NEG";
  case NODE_AND:
    return "NODE_AND";
  case NODE_OR:
    return "NODE_OR";
  case NODE_EQ:
    return "NODE_EQ";
  case NODE_NEQ:
    return "NODE_NEQ";
  case NODE_LT:
    return "NODE_LT";
  case NODE_LE:
    return "NODE_LE";
  case NODE_GT:
    return "NODE_GT";
  case NODE_GE:
    return "NODE_GE";
  case NODE_NOT:
    return "NODE_NOT";
  case NODE_IS:
    return "NODE_IS";
  case NODE_NULL:
    return "NODE_NULL";
  case NODE_ROOT:
    return "NODE_ROOT";
  default:
    fprintf(stderr, "get_node_name error at %d, %s", __LINE__, __FILE__);
    exit(1);
  }
}

Node *parse(Token *root_token) {
  Token *token = root_token;
  Node *node = new_node(NODE_ROOT, NODE_STMT, new_arr());
  Node *cur = node;

  for (; token->type != TOKEN_EOF; token = token->next) {

    if (strcasecmp(token->str, "CREATE") == 0) {
      append(cur->childs, *new_node(NODE_CREATE, NODE_CLAUSE, new_arr()));
      cur = &last(cur->childs);
      cur->token = token;
      continue;
    }

    if (strcasecmp(token->str, "TABLE") == 0) {
      append(cur->childs, *new_node(NODE_TABLE, NODE_OBJ, new_arr()));
      cur = &last(cur->childs);
      cur->token = token;
      continue;
    }

    // fprintf(stderr, "unrecognized token: %s\n", token->str);
    // exit(1);
  }
  return node;
}
