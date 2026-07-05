#include "sqldb.h"
#include <assert.h>
#include <stdlib.h>
#include <strings.h>

typedef struct {
  Expr *items;
  size_t cnt;
  size_t capa;
} ExprStack;

static void append(ExprStack *stack, Expr expr) {
  if (stack->cnt >= stack->capa) {
    if (stack->capa == 0)
      stack->capa = 16;
    else
      stack->capa *= 2;
    stack->items = realloc(stack->items, stack->capa * sizeof(*stack->items));
  }
  stack->items[stack->cnt++] = expr;
}

static Expr pop(ExprStack *stack) { return stack->items[--stack->cnt]; }
static Expr *last(ExprStack *stack) {
  return &stack->items[(assert(stack->cnt > 0), stack->cnt - 1)];
}

static int get_bindpw(Expr *expr, int i) {
  // i = 0: left hand side power
  // i = 1: right hand side power
  int power[2];
  switch (expr->type) {
  case ADD:
    power[0] = 1;
    power[1] = 2;
    return power[i];
  case SUB:
    power[0] = 1;
    power[1] = 2;
    return power[i];
  case MUL:
    power[0] = 3;
    power[1] = 4;
    return power[i];
  case DIV:
    power[0] = 3;
    power[1] = 4;
    return power[i];
  case AND:
    power[0] = 1;
    power[1] = 2;
    return power[i];
  case OR:
    power[0] = 3;
    power[1] = 4;
    return power[i];
  // case IS:
  //   return (BindingPower){.lhs = 1, .rhs = 1};
  // case NOT:
  //   return (BindingPower){.lhs = 1, .rhs = 1};
  case COMMA:
    power[0] = 1;
    power[1] = 2;
    return power[i];
  default:
    exit(2);
  }
}

Expr *new_expr(ExprTyp type) {
  Expr *expr = calloc(1, sizeof(Expr));
  expr->type = type;
  return expr;
}

Expr *parseExpr(Token *tok) {
  ExprStack stack = {0};
  Expr *root = {0};
  for (; tok->type != EOQ; tok = tok->next) {

    if (tok->type == NUMERIC) {
      if (tok->fval) {
        append(&stack, *new_expr(ATOM));
        last(&stack)->fval = tok->fval;
        last(&stack)->str = tok->str;
      }
      if (tok->ival) {
        append(&stack, *new_expr(ATOM));
        last(&stack)->ival = tok->ival;
        last(&stack)->str = tok->str;
      }
    }

    if (tok->type == OPERATOR) {
      if (strcasecmp(tok->str, "+") == 0) {
        append(&stack, *new_expr(ADD));
        last(&stack)->str = tok->str;
      }
      if (strcasecmp(tok->str, "*") == 0) {
        append(&stack, *new_expr(MUL));
        last(&stack)->str = tok->str;
      }
    }
  }
}
