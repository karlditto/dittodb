#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <regex.h> // GNU only
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// forawrd declaration
typedef struct Expr Expr;
typedef struct Clause Clause;
typedef struct Token Token;
//
// array.c
//
typedef struct {
  uint64_t *items;
  size_t cnt;
  size_t capa;
} Array;

// macro for manipluating array
// macros see arguments always separated by comma, thus expr won't work so va
// args is used. alternative is wrap argument in parentheses, va args only works
// in last arguments
// #define append(xs, ...) \
//   do { \
//     if (xs->cnt >= xs->capa) { \
//       if (xs->capa == 0) \
//         xs->capa = 256; \
//       else \
//         xs->capa *= 2; \
//       xs->items = realloc(xs->items, xs->capa * sizeof(*xs->items)); \
//     } \
//     xs->items[xs->cnt++] = __VA_ARGS__; \
//   } while (0)
// #define pop(xs) (xs->items[--(xs->cnt)])
// #define first(xs) (xs->items[assert(xs->cnt > 0), 0]) // no error handling
// #define last(xs) (xs->items[assert(xs->cnt > 0), xs->cnt - 1])

Array *new_arr();

//
// string.c
//
typedef struct {
  const char *data;
  /*
   * although cstring is already read only,
   * this const keyword is just to make sure it won't compile if data[0] = "h"
   * is issued, so it won't result in undefined behavior during runtime,
   * otherwise might be seg fault at runtime.
   */
  size_t len;
} StringView;

StringView sv(const char *cstr);

//
// tokenize.c
//
typedef enum {
  KEYWORD,    // keyword
  IDENTIFIER, // identifier
  STRING,     // string literal
  NUMERIC,    // numeric literal
  EOQ,        // EOF
  OPERATOR,   // operator
  DTYPE,      // datatype
} TokenType;

struct Token {
  TokenType type;
  Token *next;
  int64_t ival;     // if numeric token is int, this is its value
  long double fval; // if numeric token is float, this is its value
  char *loc;        // Token location
  size_t len;       // Token length
  char *str;        // null terminated string
};

Token *tokenize(char *);
char *get_token_type(const Token *);
void print_token(Token *);

//
// ast.c
//

typedef enum {
  // DDL
  CREATE,
  DROP,
  // DML
  SELECT,
  UPDATE,
  DELETE,
  INSERT,
} StmtTyp;

typedef enum {
  FROM,
  WHERE,
  SET,
  INTO,
  ORDERBY,
} ClauseTyp;

// definition of expression, consists of value and operator
typedef enum {
  ADD,
  SUB,
  MUL,
  DIV,
  AND,
  OR,
  IS,
  NOT,
  COMMA,
  LPAREN,
  RPAREN,
  ATOM // expr with a single identifier or value
} ExprTyp;

struct Expr {
  ExprTyp type;
  Expr *lhs;
  Expr *rhs;

  int64_t ival;
  long double fval;
  char *str;
};

struct Clause {
  ClauseTyp type;
  Clause *next;
};

typedef struct {
  StmtTyp type;
  Clause *clause;

} Statment;

Expr *parseExpr(Token *tok);
Expr *new_expr(ExprTyp);
