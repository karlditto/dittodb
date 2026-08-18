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
Node *parse_expr(Token **token, int min_bp) {
  Node *lhs = node_from_token(*token);
  if (lhs->type.exacttype == ADD || lhs->type.exacttype == SUB) {
    // prefix operator
    // - ( 1 + 2 )
    *token = (*token)->next;
    // - ( 1 + 2 )
    //   ^
    Node *prefix_operand = node_from_token(*token);
    if (prefix_operand->type.exacttype == ADD ||
        prefix_operand->type.exacttype == SUB) {
      // --5 invalid
      // 1+-5 valid
      fprintf(stderr, "%s\n", "chained prefix operator not supported");
      exit(EXIT_FAILURE);
    }
    if (prefix_operand->type.exacttype == LPAREN) {
      Node *paren_expr = parse_expr(token, 0);
      append(lhs->childs, paren_expr);
      lhs->expr_rhs = paren_expr;
    } else {
      append(lhs->childs, prefix_operand);
      lhs->expr_rhs = prefix_operand;
    }
  }
  if (lhs->type.exacttype == LPAREN) {
    // ( 1 + 2 ) * 3
    *token = (*token)->next;
    // ( 1 + 2 ) * 3
    //   ^
    // node_free(lhs);
    lhs = parse_expr(token, 0);
    *token = (*token)->next;
    // ( 1 + 2 ) * 3
    //         ^
    assert(strcasecmp((*token)->str, ")") == 0);
  }
  assert(lhs->type.exacttype == ADD || lhs->type.exacttype == SUB ||
         lhs->type.exacttype == ATOM || lhs->type.exacttype == MUL ||
         lhs->type.exacttype ==
             COMMA); // MUL as asterisk in select * from table
  while (true) {
    // 1 + 2 * 3
    Node *operator = node_from_token((*token)->next);
    if (operator->type.exacttype == DTYPE) {
      append(lhs->childs, operator);
      lhs->expr_lhs = operator;
      *token = (*token)->next;
      operator = node_from_token((*token)->next);
    }
    if (operator->type.exacttype == RPAREN) {
      break;
    }
    if (operator->token->type == EOQ || operator->type.nodetype == STATEMMENT ||
        operator->type.nodetype == CLAUSE) {
      break;
    }

    assert(operator->token->type == OPERATOR);
    int l_bp = bindpower_lookup(operator->token).lhs;
    int r_bp = bindpower_lookup(operator->token).rhs;
    if (l_bp < min_bp) {
      break;
    }
    // 1 + 2 * 3
    //   ^
    *token = (*token)->next;
    // 1 + 2 * 3
    //     ^
    *token = (*token)->next;
    Node *rhs = parse_expr(token, r_bp);
    append(operator->childs, lhs);
    append(operator->childs, rhs);
    operator->expr_lhs = lhs;
    operator->expr_rhs = rhs;

    lhs = operator;
  }
  return lhs;
}

Node *parse_expr2(ExactType type, Token **token, int min_bp) {
  switch (type) {

  case TABLE: { // add bracket to avoid annoying warning msg
    Node *lhs = node_from_token(*token);

    // deal with parentheses
    if (lhs->type.exacttype == LPAREN) {
      *token = (*token)->next; // skip "("
      lhs = parse_expr2(TABLE, token, 0);
      *token = (*token)->next; // skip ")"
      if (strcasecmp((*token)->str, ")") != 0) {
        fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
        return NULL;
      }
    }

    while (true) {
      Node *operator = node_from_token((*token)->next);

      if (operator->type.exacttype == DTYPE) {

        if (strcasecmp(operator->token->str, "CHAR") == 0) {
          append(operator->childs, lhs);
          operator->expr_lhs = lhs;
          *token = (*token)->next; // char(32)
                                   // ^
          *token = (*token)->next; // char(32)
                                   //     ^
          *token = (*token)->next; // char(32)
                                   //      ^
          Node *size = node_from_token(*token);
          append(operator->childs, size);
          operator->expr_rhs = size;
          lhs = operator;
          *token = (*token)->next;                    // char(32)
                                                      //        ^
          operator = node_from_token((*token)->next); // ","
        } else {
          append(operator->childs, lhs);
          operator->expr_lhs = lhs;
          lhs = operator;
          *token = (*token)->next; // pointer moves to datatype
          operator = node_from_token((*token)->next);
        }
      }

      if (operator->type.exacttype == RPAREN) {
        break;
      }

      if (operator->type.nodetype != EXPRESSION) {
        break;
      }

      if (operator->token->type != OPERATOR) {
        fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
        return NULL;
      } // stop if not expression

      int l_bp = bindpower_lookup(operator->token).lhs;
      int r_bp = bindpower_lookup(operator->token).rhs;
      if (l_bp < min_bp) {
        break;
      }
      *token = (*token)->next;
      *token = (*token)->next; // pointer moves to rhs atom
      Node *rhs = parse_expr2(TABLE, token, r_bp);
      append(operator->childs, lhs);
      append(operator->childs, rhs);
      operator->expr_lhs = lhs;
      operator->expr_rhs = rhs;

      lhs = operator;
    }
    return lhs;
    break;
  }

  case SELECT: { // add bracket to avoid annoying warning msg
    Node *lhs = node_from_token(*token);

    if (lhs->type.exacttype == ADD || lhs->type.exacttype == SUB) {
      Node *prefix_operator = node_from_token(*token);
      *token = (*token)->next;
      Node *prefix_operand = parse_expr2(SELECT, token, 0);
      append(prefix_operator->childs, prefix_operand);
      prefix_operator->expr_lhs = prefix_operand;
      lhs = prefix_operator;
    }

    // deal with parentheses
    if (lhs->type.exacttype == LPAREN) {
      *token = (*token)->next; // skip "("
      lhs = parse_expr2(SELECT, token, 0);
      *token = (*token)->next; // skip ")"
      if (strcasecmp((*token)->str, ")") != 0) {
        fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
        return NULL;
      }
    }

    while (true) {
      Node *operator = node_from_token((*token)->next);

      if (operator->type.exacttype == RPAREN) {
        break;
      }

      if (operator->type.nodetype != EXPRESSION) {
        break;
      }

      if (operator->token->type != OPERATOR) {
        fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
        return NULL;
      } // stop if not expression

      int l_bp = bindpower_lookup(operator->token).lhs;
      int r_bp = bindpower_lookup(operator->token).rhs;
      if (l_bp < min_bp) {
        break;
      }
      *token = (*token)->next;
      *token = (*token)->next; // pointer moves to rhs atom
      Node *rhs = parse_expr2(SELECT, token, r_bp);
      append(operator->childs, lhs);
      append(operator->childs, rhs);
      operator->expr_lhs = lhs;
      operator->expr_rhs = rhs;

      lhs = operator;
    }
    return lhs;
    break;
  }

    // copypaste from select branch
  case FROM: { // add bracket to avoid annoying warning msg
    Node *lhs = node_from_token(*token);

    if (lhs->type.exacttype == ADD || lhs->type.exacttype == SUB) {
      Node *prefix_operator = node_from_token(*token);
      *token = (*token)->next;
      Node *prefix_operand = parse_expr2(FROM, token, 0);
      append(prefix_operator->childs, prefix_operand);
      prefix_operator->expr_lhs = prefix_operand;
      lhs = prefix_operator;
    }

    // deal with parentheses
    if (lhs->type.exacttype == LPAREN) {
      *token = (*token)->next; // skip "("
      lhs = parse_expr2(FROM, token, 0);
      *token = (*token)->next; // skip ")"
      if (strcasecmp((*token)->str, ")") != 0) {
        fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
        return NULL;
      }
    }

    while (true) {
      Node *operator = node_from_token((*token)->next);

      if (operator->type.exacttype == RPAREN) {
        break;
      }

      if (operator->type.nodetype != EXPRESSION) {
        break;
      }

      if (operator->token->type != OPERATOR) {
        fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
        return NULL;
      } // stop if not expression

      int l_bp = bindpower_lookup(operator->token).lhs;
      int r_bp = bindpower_lookup(operator->token).rhs;
      if (l_bp < min_bp) {
        break;
      }
      *token = (*token)->next;
      *token = (*token)->next; // pointer moves to rhs atom
      Node *rhs = parse_expr2(FROM, token, r_bp);
      append(operator->childs, lhs);
      append(operator->childs, rhs);
      operator->expr_lhs = lhs;
      operator->expr_rhs = rhs;

      lhs = operator;
    }
    return lhs;
    break;
  }
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

      case CREATE:
        cur_node = root;
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip "create"

        node = node_from_token(token);
        ExactType obj_typ;
        if (node->type.nodetype != OBJECT) {
          fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
          return NULL;
        }
        obj_typ = node->type.exacttype;
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip "table/index"

        node = node_from_token(token);
        if (node->token->type != IDENTIFIER) {
          fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
          return NULL;
        }
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip tablename

        node = node_from_token(token);
        if (node->type.exacttype == LPAREN && obj_typ == TABLE) {
          node = parse_expr2(TABLE, &token, 0);
          if (!node) {
            fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
            return NULL;
          }
          append(cur_node->childs, node);
        } else if (node->type.exacttype == ON && obj_typ == INDEX) {
          fprintf(stderr, "NOT IMPLEMENTED\n");
          return NULL;
        }

        break;

      case INSERT:
        cur_node = root;
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next;
        node = node_from_token(token);
        if (node->type.exacttype != INTO) {
          fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
          return NULL;
        }
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        break;

      case SELECT:
        cur_node = root;
        append(cur_node->childs, node);
        cur_node = last(cur_node->childs);
        token = token->next; // skip "select"
                             //
        node = node_from_token(token);
        if (node->type.nodetype != EXPRESSION) {
          fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
          return NULL;
        }
        node = parse_expr2(SELECT, &token, 0);
        if (!node) {
          fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
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
          fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
          return NULL;
        }
        node = parse_expr2(FROM, &token, 0);
        if (!node) {
          fprintf(stderr, "syntax error at %s:%d\n", __FILE__, __LINE__);
          return NULL;
        }
        append(cur_node->childs, node);
        break;
      }
    }
  }

  return root;
}
