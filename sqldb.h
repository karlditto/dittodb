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

typedef struct Token Token;
typedef struct Node Node;
//
// array.c
//
typedef struct {
  Node *items;
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
// #define at(xs, index) (xs->items[assert(xs->cnt > index), index])

void append(Array *, Node *);
Node *pop(Array *);
Node *first(Array *);
Node *last(Array *);
Node *at(Array *, int);

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
  DATATYPE,   // datatype
  TOKENCOUNT
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
  // statement type
  ROOT,
  // DDL
  CREATE,
  DROP,
  // DML
  SELECT,
  UPDATE,
  DELETE,
  INSERT,

  // clause type
  FROM,
  WHERE,
  SET,
  INTO,
  ORDER,
  BY,
  VALUES,

  // expression type
  ADD,
  SUB,
  MUL,
  DIV,
  AND,
  OR,
  IS,
  NOT,
  COMMA,
  EQ,
  LPAREN,
  RPAREN,
  ATOM,
  DTYPE,

  // object catalog
  TABLE,
  INDEX,

  TYPECOUNT,
} ExactType;

typedef enum { STATEMMENT, EXPRESSION, CLAUSE, OBJECT, NODETYPECOUNT } NodeType;

typedef struct {
  NodeType nodetype;
  ExactType exacttype;
} Type;

struct Node {
  Type type;

  Array *childs;
  Node *expr_lhs;
  Node *expr_rhs; // TODO: migrate to lhs, rhs
  Token *token;
};

typedef struct {
  int lhs;
  int rhs;
} BindPower;

Node *parse(Token *);
void print_ast(Node *, int);
