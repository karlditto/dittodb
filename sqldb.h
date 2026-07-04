#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// forawrd declaration
typedef struct Node Node;
typedef struct Token Token;
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
#define append(xs, ...)                                                        \
  do {                                                                         \
    if (xs->cnt >= xs->capa) {                                                 \
      if (xs->capa == 0)                                                       \
        xs->capa = 256;                                                        \
      else                                                                     \
        xs->capa *= 2;                                                         \
      xs->items = realloc(xs->items, xs->capa * sizeof(*xs->items));           \
    }                                                                          \
    xs->items[xs->cnt++] = __VA_ARGS__;                                        \
  } while (0)
#define pop(xs) (xs->items[--(xs->cnt)])
#define first(xs) (xs->items[assert(xs->cnt > 0), 0]) // no error handling
#define last(xs) (xs->items[assert(xs->cnt > 0), xs->cnt - 1])

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
  TOKEN_KYWD,   // keyword
  TOKEN_IDTF,   // identifier
  TOKEN_COMMA,  // ,
  TOKEN_STR,    // string literal
  TOKEN_NUM,    // numeric literal
  TOKEN_EOF,    // EOF
  TOKEN_OP,     // operator
  TOKEN_STAR,   // *
  TOKEN_LPAREN, // (
  TOKEN_RPAREN, // )
  TOKEN_DTYPE,  // datatype
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
char *get_token_name(const Token *);

// AST node
typedef enum {
  NODE_CREATE,
  NODE_SELECT,
  NODE_INSERT,
  NODE_DELETE,
  NODE_FROM,
  NODE_TABLE,
  NODE_ADD,
  NODE_SUB,
  NODE_MUL,
  NODE_DIV,
  NODE_NEG,
  NODE_AND,
  NODE_OR,
  NODE_EQ,
  NODE_NEQ,
  NODE_LT,
  NODE_LE,
  NODE_GT,
  NODE_GE,
  NODE_NOT,
  NODE_IS,
  NODE_NULL,

  NODE_ROOT
} NodeName;

typedef enum {
  NODE_CLAUSE,
  NODE_STMT,
  NODE_TERM, // select term
  NODE_BIOP, // binary operator class
  NODE_UOP,  // unary operator class
  NODE_STR,  // string literal
  NODE_NUM,  // numeric literal
  NODE_ITDF, // identifier
  NODE_OBJ,  // object type
} NodeClass;

struct Node {
  NodeName name;
  NodeClass nodeclass;
  Array *childs;
  Token *token;

  int64_t ival;
  long double fval;
};

Node *parse(Token *);
Node *new_node(NodeName, NodeClass, Array *);
char *get_node_name(Node *);
