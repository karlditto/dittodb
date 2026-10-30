#include "hashmap.h"
#include "parse.h"
#include "sqldb.h"
#include "stackmachine.h"
#include <stddef.h>
typedef enum {
  SQL_TYPE_INT,
  SQL_TYPE_FLOAT,
  SQL_TYPE_CHAR,
  SQL_TYPE_COUNT,
} SQL_TYPE;

typedef struct {
  VM_Instruction *items;
  size_t size;
  size_t used;
} SQL_Program;

typedef struct {
  Node *items;
  size_t size;
  size_t used;
} Ast_Array;

typedef struct {
  char colname[OBJ_NAME_LEN];
  char type[8];
  int len;
} SQL_Col_Def;

typedef struct {
  SQL_Col_Def *items;
  size_t size;
  size_t used;
} SQL_Col_Def_Array;

#define sql_push(xs, x)                                                        \
  do {                                                                         \
    if (xs.used >= xs.size) {                                                  \
      if (xs.size == 0)                                                        \
        xs.size = 32;                                                          \
      else                                                                     \
        xs.size *= 2;                                                          \
      xs.items = realloc(xs.items, xs.size * sizeof(*xs.items));               \
    }                                                                          \
    xs.items[xs.used++] = x;                                                   \
  } while (0)

SQL_Program sql_program(Node *ast);
SQL_Program sql_gen_prog_from_ast_arr(Ast_Array ast);
