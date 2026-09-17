#include "sqldb.h"
#include <assert.h>
#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

extern Region mem_ast;

void node_free(Node *node) {
  Node *tmp;
  for (size_t i = 0; i < node->childs->cnt; i++) {
    tmp = &node->childs->items[i];
    node_free(tmp);
  }
  free(node->childs->items);
}

// static Node *new_node(ExactType exacttype, NodeType nodetype) {
//   Node *node = calloc(1, sizeof(Node));
//   node->type.nodetype = nodetype;
//   node->type.exacttype = exacttype;
//   node->childs = calloc(1, sizeof(Array));
//   return node;
// }

static Node *new_node(ExactType exacttype, NodeType nodetype) {
  Node *node = region_alloc(&mem_ast, sizeof(Node));
  node->type.nodetype = nodetype;
  node->type.exacttype = exacttype;
  node->childs = region_alloc(&mem_ast, sizeof(Array));
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
  exacttypestr[ROOT] = "ROOT";
  exacttypestr[FROM] = "FROM";
  exacttypestr[WHERE] = "WHERE";
  exacttypestr[SET] = "SET";
  exacttypestr[INTO] = "INTO";
  exacttypestr[ORDER] = "ORDER";
  exacttypestr[BY] = "BY";
  exacttypestr[VALUES] = "VALUES";
  exacttypestr[ADD] = "ADD";
  exacttypestr[SUB] = "SUB";
  exacttypestr[MUL] = "MUL";
  exacttypestr[DIV] = "DIV";
  exacttypestr[AND] = "AND";
  exacttypestr[OR] = "OR";
  exacttypestr[IS] = "IS";
  exacttypestr[NOT] = "NOT";
  exacttypestr[COMMA] = "COMMA";
  exacttypestr[EQ] = "EQ";
  exacttypestr[LPAREN] = "LPAREN";
  exacttypestr[RPAREN] = "RPAREN";
  exacttypestr[ATOM] = "ATOM";
  exacttypestr[DTYPE] = "DTYPE";
  exacttypestr[TABLE] = "TABLE";
  exacttypestr[INDEX] = "INDEX";
  return exacttypestr[node->type.exacttype];
}

char *get_node_type(Node *node) {
  char *nodetypestr[NODETYPECOUNT];
  nodetypestr[STATEMMENT] = "STATEMMENT";
  nodetypestr[EXPRESSION] = "EXPRESSION";
  nodetypestr[CLAUSE] = "CLAUSE";
  nodetypestr[OBJECT] = "OBJECT";
  return nodetypestr[node->type.nodetype];
}

void print_ast(Node *node, int depth) {
  assert(node);
  printf("%-3d| %*s %s (%s %s)\n", depth, 1 * depth, "|", node->token->str,
         get_exact_type(node), get_node_type(node));
  for (size_t i = 0; i < node->childs->cnt; i++) {
    Node *cur_node = at(node->childs, i);
    print_ast(cur_node, depth + 1);
  }
}

static BindPower bindpower_lookup(Token *token) {
  if (strcasecmp(token->str, "+") == 0) {
    return (BindPower){.lhs = 3, .rhs = 4};
  }
  if (strcasecmp(token->str, "-") == 0) {
    return (BindPower){.lhs = 3, .rhs = 4};
  }
  if (strcasecmp(token->str, "*") == 0) {
    return (BindPower){.lhs = 5, .rhs = 6};
  }
  if (strcasecmp(token->str, "/") == 0) {
    return (BindPower){.lhs = 5, .rhs = 6};
  }
  if (strcasecmp(token->str, "(") == 0) {
    return (BindPower){.lhs = 5, .rhs = 6};
  }
  if (strcasecmp(token->str, ")") == 0) {
    return (BindPower){.lhs = 5, .rhs = 6};
  }
  if (strcasecmp(token->str, ",") == 0) {
    return (BindPower){.lhs = 1, .rhs = 2};
  }
  if (strcasecmp(token->str, "=") == 0) {
    return (BindPower){.lhs = 2, .rhs = 1};
  }
  fprintf(stderr, "unknown operator in bind power lookup phase");
  exit(EXIT_FAILURE);
  return (BindPower){.lhs = 0, .rhs = 0};
}

static Node *node_from_token(Token *token) {
  Node *node = new_node(ROOT, STATEMMENT);
  node->token = token;
  switch (token->type) {
  case TOKENCOUNT:
    break;
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
    if (strcasecmp(token->str, "(") == 0) {
      node->type.exacttype = LPAREN;
      return node;
    }
    if (strcasecmp(token->str, ")") == 0) {
      node->type.exacttype = RPAREN;
      return node;
    }
    if (strcasecmp(token->str, ",") == 0) {
      node->type.exacttype = COMMA;
      return node;
    }
    if (strcasecmp(token->str, "=") == 0) {
      node->type.exacttype = EQ;
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
    if (strcasecmp(token->str, "VALUES") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = VALUES;
      return node;
    }
    if (strcasecmp(token->str, "ON") == 0) {
      node->type.nodetype = CLAUSE;
      node->type.exacttype = ON;
      return node;
    }
    if (strcasecmp(token->str, "TABLE") == 0) {
      node->type.nodetype = OBJECT;
      node->type.exacttype = TABLE;
      return node;
    }
    if (strcasecmp(token->str, "INDEX") == 0) {
      node->type.nodetype = OBJECT;
      node->type.exacttype = INDEX;
      return node;
    }
    fprintf(stderr, "cannot infer nodetype from token at %s:%d\n", __FILE__,
            __LINE__);
    exit(EXIT_FAILURE);
    return node;
  }
  return NULL;
}

// pratt parsing of expression
//    a   +   b   *   c   +    d             min bind power = 0
//    ^  1 2     3 4     1 2
//   [a   +]  b   *   c   +    d             min bind power = R_BP("+")=2
//            ^  3 4     1 2
//   [a   +]  b   *   c   +    d             L_BP('*')=3 > 2;
//   [a   +] [b   *   c   +    d]            recursively execute parse on 2nd
//   bracket [a   +] [b   *   c]  +    d] [a   +][[b   *   c]  +]   d [a   +][[b
//   *   c]  +    d] [a   +][ b   *   c   +    d] [a   +   b   *   c   +    d]
//   then the expression is solved
//
//
// select a+1,b,c
Node *parse_select(Token **token, int min_bp) {
  Node *lhs = node_from_token(*token); // select a+1,b,c
                                       //        ^
  // case: -1, +2
  if (lhs->type.exacttype == ADD || lhs->type.exacttype == SUB) {
    Node *prefix_operator = node_from_token(*token); // case: -1
                                                     //       ^
    *token = (*token)->next;                         // -1
                                                     //  ^
    Node *prefix_operand = parse_select(token, 0);
    if (!prefix_operand) {
      return NULL;
    }
    append(prefix_operator->childs, prefix_operand);
    prefix_operator->expr_lhs = prefix_operand;
    lhs = prefix_operator;
  }

  // deal with parentheses
  if (lhs->type.exacttype == LPAREN) {
    *token = (*token)->next; // skip "("
    lhs = parse_select(token, 0);
    if (!lhs) {
      return NULL;
    }
    *token = (*token)->next; // skip ")"
    if (strcasecmp((*token)->str, ")") != 0) {
      fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
      fprintf(stderr, "[ERROR] \")\" expected, but get \"%s\"\n",
              (*token)->str);
      return NULL;
    }
  }

  if (node_from_token(*token)->type.nodetype != EXPRESSION) {
    fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
    fprintf(stderr, "[ERROR] expr expected, but get \"%s\"\n", (*token)->str);
    return NULL;
  }
  while (true) {
    Node *operator = node_from_token((*token)->next);

    if (operator->type.exacttype == LPAREN) {
      fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
      fprintf(stderr,
              "[ERROR] This operator should not be used like this, current "
              "token: \"%s\"\n",
              operator->token->str);
      return NULL;
    }
    if (operator->type.exacttype == RPAREN) {
      break;
    }

    if (operator->type.nodetype != EXPRESSION) {
      break;
    }

    if (operator->token->type != OPERATOR) {
      fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
      fprintf(stderr, "[ERROR] operator expected at token: \"%s\"\n",
              operator->token->str);
      return NULL;
    } // stop if not expression

    int l_bp = bindpower_lookup(operator->token).lhs;
    int r_bp = bindpower_lookup(operator->token).rhs;
    if (l_bp < min_bp) {
      break;
    }
    *token = (*token)->next;
    *token = (*token)->next; // pointer moves to rhs atom
    Node *rhs = parse_select(token, r_bp);
    if (!rhs) {
      return NULL;
    }
    append(operator->childs, lhs);
    append(operator->childs, rhs);
    operator->expr_lhs = lhs;
    operator->expr_rhs = rhs;

    lhs = operator;
  }
  return lhs;
}

Node *parse_from(Token **token, int min_bp) {
  Node *lhs = node_from_token(*token);

  if (lhs->type.exacttype == ADD || lhs->type.exacttype == SUB) {
    Node *prefix_operator = node_from_token(*token);
    *token = (*token)->next;
    Node *prefix_operand = parse_from(token, 0);
    if (!prefix_operand) {
      return NULL;
    }
    append(prefix_operator->childs, prefix_operand);
    prefix_operator->expr_lhs = prefix_operand;
    lhs = prefix_operator;
  }

  // deal with parentheses
  if (lhs->type.exacttype == LPAREN) {
    *token = (*token)->next; // skip "("
    lhs = parse_from(token, 0);
    if (!lhs) {
      return NULL;
    }
    *token = (*token)->next; // skip ")"
    if (strcasecmp((*token)->str, ")") != 0) {
      fprintf(stderr, "[ERROR] at FROM parsing phase\n");
      fprintf(stderr, "\")\" expected, but get \"%s\"\n", (*token)->str);
      return NULL;
    }
  }

  if (node_from_token(*token)->type.nodetype != EXPRESSION) {
    fprintf(stderr, "[ERROR] at FROM parsing phase\n");
    fprintf(stderr, "[ERROR] expr expected, but get \"%s\"\n", (*token)->str);
    return NULL;
  }
  while (true) {
    Node *operator = node_from_token((*token)->next);

    if (operator->type.exacttype == LPAREN) {
      fprintf(stderr, "[ERROR] at FROM parsing phase\n");
      fprintf(stderr,
              "[ERROR] This operator should not be used like this, current "
              "token: \"%s\"\n",
              operator->token->str);
      return NULL;
    }
    if (operator->type.exacttype == RPAREN) {
      break;
    }

    if (operator->type.nodetype != EXPRESSION) {
      break;
    }

    if (operator->type.exacttype != COMMA) {
      fprintf(stderr, "[ERROR] at FROM parsing phase\n");
      fprintf(stderr, "[ERROR] operator \",\" expected at token: \"%s\"\n",
              operator->token->str);
      return NULL;
    }

    int l_bp = bindpower_lookup(operator->token).lhs;
    int r_bp = bindpower_lookup(operator->token).rhs;
    if (l_bp < min_bp) {
      break;
    }
    *token = (*token)->next;
    *token = (*token)->next; // pointer moves to rhs atom
    Node *rhs = parse_from(token, r_bp);
    if (!rhs) {
      return NULL;
    }
    append(operator->childs, lhs);
    append(operator->childs, rhs);
    operator->expr_lhs = lhs;
    operator->expr_rhs = rhs;

    lhs = operator;
  }
  return lhs;
}

Node *parse_create(Token **token, int min_bp) {
  Node *lhs = node_from_token(*token);

  // deal with parentheses
  if (lhs->type.exacttype == LPAREN) {
    *token = (*token)->next; // skip "("
    lhs = parse_create(token, 0);
    if (!lhs) {
      return NULL;
    }
    *token = (*token)->next; // skip ")"
    if (strcasecmp((*token)->str, ")") != 0) {
      fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
      fprintf(stderr, "[ERROR] \")\" expected, but get \"%s\"\n",
              (*token)->str);
      return NULL;
    }
  }

  Node *colname = node_from_token(*token);
  if (colname->token->type == IDENTIFIER) {
    *token = (*token)->next; // token to datatype
    Node *datatype = node_from_token(*token);
    if (datatype->type.exacttype == DTYPE) {
      append(datatype->childs, lhs);
      datatype->expr_lhs = lhs;
      lhs = datatype;
    } else {
      fprintf(stderr, "[ERROR] at CREATE parsing phase\n");
      fprintf(stderr, "[ERROR] datatype expected, but get \"%s\"\n",
              (*token)->str);
      return NULL;
    }
  }

  if (node_from_token(*token)->type.nodetype != EXPRESSION) {
    fprintf(stderr, "[ERROR] at CREATE parsing phase\n");
    fprintf(stderr, "[ERROR] expr expected, but get \"%s\"\n", (*token)->str);
    return NULL;
  }
  while (true) {
    Node *operator = node_from_token((*token)->next);

    if (operator->type.exacttype == LPAREN) {
      fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
      fprintf(stderr,
              "[ERROR] This operator should not be used like this, current "
              "token: \"%s\"\n",
              operator->token->str);
      return NULL;
    }
    if (operator->type.exacttype == RPAREN) {
      break;
    }

    if (operator->type.nodetype != EXPRESSION) {
      break;
    }

    if (operator->type.exacttype != COMMA) {
      fprintf(stderr, "[ERROR] at FROM parsing phase\n");
      fprintf(stderr, "[ERROR] operator \",\" expected at token: \"%s\"\n",
              operator->token->str);
      return NULL;
    }

    int l_bp = bindpower_lookup(operator->token).lhs;
    int r_bp = bindpower_lookup(operator->token).rhs;
    if (l_bp < min_bp) {
      break;
    }
    *token = (*token)->next;
    *token = (*token)->next; // pointer moves to rhs atom
    Node *rhs = parse_create(token, r_bp);
    if (!rhs) {
      return NULL;
    }
    append(operator->childs, lhs);
    append(operator->childs, rhs);
    operator->expr_lhs = lhs;
    operator->expr_rhs = rhs;

    lhs = operator;
  }
  return lhs;
}

Node *parse_expr(ExactType type, Token **token) {
  switch (type) {

  case SELECT: { // add bracket to avoid annoying warning msg
    return parse_select(token, 0);
    break;
  }

    // copypaste from select branch
  case FROM: { // add bracket to avoid annoying warning msg
    return parse_from(token, 0);
    break;
  }

  case CREATE:
    return parse_create(token, 0);
    break;
  }

  return NULL;
}

Node *parse(Token *token) {
  Node *root = new_node(ROOT, STATEMMENT);
  // root->token = &(Token){.str = "ROOT OF QUERY"}; // stack-use-after-return
  root->token = new_token(KEYWORD, "ROOT OF QUERY", 0);
  root->token->str = "ROOT OF QUERY";
  Node *cur_node = root;

  for (; token->type != EOQ; token = token->next) {
    Node *node = node_from_token(token);

    if (node->type.nodetype == STATEMMENT || node->type.nodetype == CLAUSE) {

      switch (node->type.exacttype) {

      case SELECT:
        cur_node = root;
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip "select"
        node = node_from_token(token);
        if (node->type.nodetype != EXPRESSION) {
          fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
          fprintf(stderr, "[ERROR] expr expected, but get \"%s\"\n",
                  node->token->str);
          return NULL;
        }
        node = parse_expr(SELECT, &token);
        if (!node) {
          fprintf(stderr, "[ERROR] at SELECT parsing phase\n");
          fprintf(stderr, "[ERROR] SELECT stmt parse error\n");
          return NULL;
        }
        append(cur_node->childs, node);
        break;

      case FROM:
        cur_node = root;
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip "from"
        node = node_from_token(token);
        if (node->type.nodetype != EXPRESSION) {
          fprintf(stderr, "[ERROR] at FROM parsing phase\n");
          fprintf(stderr, "[ERROR] expr expected, but get \"%s\"\n",
                  node->token->str);
          return NULL;
        }
        node = parse_expr(FROM, &token);
        if (!node) {
          fprintf(stderr, "[ERROR] at FROM parsing phase\n");
          fprintf(stderr, "[ERROR] FROM clause parse error\n");
          return NULL;
        }
        append(cur_node->childs, node);
        break;

      case CREATE:
        cur_node = root;
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip "create"
        node = node_from_token(token);
        if (node->type.exacttype != TABLE) {
          fprintf(stderr, "[ERROR] at CREATE parsing phase\n");
          fprintf(stderr, "[ERROR] should be a TABLE, but get \"%s\"\n",
                  node->token->str);
          return NULL;
        }

        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip "table"
        node = node_from_token(token);
        if (node->token->type != IDENTIFIER) {
          fprintf(stderr, "[ERROR] at CREATE parsing phase\n");
          fprintf(stderr, "[ERROR] should be identifier, but get \"%s\"\n",
                  node->token->str);
          return NULL;
        }
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip tablename

        node = node_from_token(token);
        if (node->type.nodetype != EXPRESSION) {
          fprintf(stderr, "[ERROR] at CREATE parsing phase\n");
          fprintf(stderr, "[ERROR] expr expected, but get \"%s\"\n",
                  node->token->str);
          return NULL;
        }
        node = parse_expr(CREATE, &token);
        if (!node) {
          fprintf(stderr, "[ERROR] at CREATE parsing phase\n");
          fprintf(stderr, "[ERROR] CREATE clause parse error\n");
          return NULL;
        }
        append(cur_node->childs, node);
        break;

      default:
        fprintf(stderr,
                "[ERROR] token not consumed by parser, current token: \"%s\"\n",
                node->token->str);
        exit(1);
      }

    } else {
      fprintf(stderr, "[ERROR] is this SQL?\n");
      return NULL;
    }
  }

  return root;
}
