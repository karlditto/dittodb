#include "sqldb.h"
#include <stddef.h>

static Node *new_node(ExactType exacttype, NodeType nodetype) {
  Node *node = calloc(1, sizeof(Node));
  node->type.nodetype = nodetype;
  node->type.exacttype = exacttype;
  node->childs = calloc(1, sizeof(Array));
  return node;
}

static char *get_exact_type(Node *node) {
  char *exacttypestr[TYPECOUNT];
  exacttypestr[CREATE] = "CREATE";
  exacttypestr[DROP] = "DROP";
  exacttypestr[SELECT] = "SELECT";
  exacttypestr[UPDATE] = "UPDATE";
  exacttypestr[DELETE] = "DELETE";
  exacttypestr[INSERT] = "INSERT";
  exacttypestr[FROM] = "FROM";
  exacttypestr[WHERE] = "WHERE";
  exacttypestr[SET] = "SET";
  exacttypestr[INTO] = "INTO";
  exacttypestr[ORDER] = "ORDER";
  exacttypestr[BY] = "BY";
  exacttypestr[ADD] = "ADD";
  exacttypestr[SUB] = "SUB";
  exacttypestr[MUL] = "MUL";
  exacttypestr[DIV] = "DIV";
  exacttypestr[AND] = "AND";
  exacttypestr[OR] = "OR";
  exacttypestr[IS] = "IS";
  exacttypestr[NOT] = "NOT";
  exacttypestr[COMMA] = "COMMA";
  exacttypestr[LPAREN] = "LPAREN";
  exacttypestr[RPAREN] = "RPAREN";
  exacttypestr[ATOM] = "ATOM";
  exacttypestr[DTYPE] = "DTYPE";
  exacttypestr[ROOT] = "ROOT";
  return exacttypestr[node->type.exacttype];
}

char *get_node_type(Node *node) {
  char *nodetypestr[NODETYPECOUNT];
  nodetypestr[STATEMMENT] = "STATEMMENT";
  nodetypestr[EXPRESSION] = "EXPRESSION";
  nodetypestr[CLAUSE] = "CLAUSE";
  return nodetypestr[node->type.nodetype];
}

void print_ast(Node *node, int depth) {
  printf("%d %s %s\n", depth, get_exact_type(node), get_node_type(node));
  for (size_t i = 0; i < node->childs->cnt; i++) {
    Node *cur_node = at(node->childs, i);
    print_ast(cur_node, depth + 1);
  }
}

static BindPower bindpower_lookup(Token *token) {
  if (strcasecmp(token->str, "+") == 0) {
    return (BindPower){.lhs = 1, .rhs = 2};
  }
  if (strcasecmp(token->str, "-") == 0) {
    return (BindPower){.lhs = 1, .rhs = 2};
  }
  if (strcasecmp(token->str, "*") == 0) {
    return (BindPower){.lhs = 3, .rhs = 4};
  }
  if (strcasecmp(token->str, "/") == 0) {
    return (BindPower){.lhs = 3, .rhs = 4};
  }
  fprintf(stderr, "unknown operator in bind power lookup phase");
  exit(EXIT_FAILURE);
  return (BindPower){.lhs = 0, .rhs = 0};
}

static Node *node_from_token(Token *token) {
  Node *node = new_node(ROOT, STATEMMENT);
  node->token = token;
  switch (token->type) {
  case EOQ:
    return node;
  case DATATYPE:
    node->type.nodetype = EXPRESSION;
    node->type.exacttype = DTYPE;
    return node;
  case STRING:
    node->type.nodetype = EXPRESSION;
    node->type.exacttype = ATOM;
    return node;
  case NUMERIC:
    node->type.nodetype = EXPRESSION;
    node->type.exacttype = ATOM;
    return node;
  case IDENTIFIER:
    node->type.nodetype = EXPRESSION;
    node->type.exacttype = ATOM;
    return node;
  case OPERATOR:
    node->type.nodetype = EXPRESSION;
    if (strcasecmp(token->str, "+") == 0) {
      node->type.exacttype = ADD;
      return node;
    }
    if (strcasecmp(token->str, "-") == 0) {
      node->type.exacttype = SUB;
      return node;
    }
    if (strcasecmp(token->str, "*") == 0) {
      node->type.exacttype = MUL;
      return node;
    }
    if (strcasecmp(token->str, "/") == 0) {
      node->type.exacttype = DIV;
      return node;
    }
    fprintf(stderr, "unknown operator at %s:%d", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
    return node;
  case KEYWORD:
    if (strcasecmp(token->str, "CREATE") == 0) {
      node->type.nodetype = STATEMMENT;
      node->type.exacttype = CREATE;
      return node;
    }
    if (strcasecmp(token->str, "DROP") == 0) {
      node->type.nodetype = STATEMMENT;
      node->type.exacttype = DROP;
      return node;
    }
    if (strcasecmp(token->str, "SELECT") == 0) {
      node->type.nodetype = STATEMMENT;
      node->type.exacttype = SELECT;
      return node;
    }
    if (strcasecmp(token->str, "UPDATE") == 0) {
      node->type.nodetype = STATEMMENT;
      node->type.exacttype = UPDATE;
      return node;
    }
    if (strcasecmp(token->str, "INSERT") == 0) {
      node->type.nodetype = STATEMMENT;
      node->type.exacttype = INSERT;
      return node;
    }
    if (strcasecmp(token->str, "DELETE") == 0) {
      node->type.nodetype = STATEMMENT;
      node->type.exacttype = DELETE;
      return node;
    }
    if (strcasecmp(token->str, "FROM") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = FROM;
      return node;
    }
    if (strcasecmp(token->str, "WHERE") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = WHERE;
      return node;
    }
    if (strcasecmp(token->str, "SET") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = SET;
      return node;
    }
    if (strcasecmp(token->str, "INTO") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = INTO;
      return node;
    }
    if (strcasecmp(token->str, "ORDER") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = ORDER;
      return node;
    }
    if (strcasecmp(token->str, "BY") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = BY;
      return node;
    }
    fprintf(stderr, "unknown operator at %s:%d", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
    return node;
  }
}

static Node *pratt_parse(Token *token) {

  for (; token->type != EOQ; token = token->next) {
    if (token->type == token->next->type) {
      break;
    }
  }
}

static Node *parse_expr(Token *token) {

  for (; token->type != EOQ; token = token->next) {
  }
}

Node *parse(Token *token) {
  Node *root = new_node(ROOT, STATEMMENT);
  Node *cur_node = root;

  for (; token->type != EOQ; token = token->next) {

    append(cur_node->childs, node_from_token(token));
    cur_node = last(cur_node->childs);
  }

  return root;
}
