#pragma once
#include "hashmap.h"
#include "parse.h"
#include "sqldb.h"
#include "stackmachine.h"
#include <stddef.h>
#include <stdint.h>
typedef enum {
  SQL_TYPE_NOTYPE,
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
  TokenType type;
  int len;
} SQL_Col_Def;

typedef struct {
  SQL_Col_Def *items;
  size_t size;
  size_t used;
} SQL_Col_Def_Array;

typedef struct {
  TokenType datatype;
  union {
    char *as_str;
    int64_t as_int;
    double as_float;
  } val;
} SQL_Insert_Val;

typedef struct {
  SQL_Insert_Val *items;
  size_t size;
  size_t used;
} SQL_Insert_Val_Array;

typedef struct {
  Node **items;
  size_t size;
  size_t used;
} SQL_Table_List;

typedef struct {
  Node *node;
  int cursor;
  int slotcnt;
} SQL_Table_n_Cursor;

typedef struct {
  SQL_Table_n_Cursor *items;
  size_t size;
  size_t used;
} SQL_Table_List_n_Cursor;

typedef struct {
  char *tabname;
  char *colname;
  SQL_TYPE type;
  size_t len;
} SQL_Cartesian_Product_Layout;

typedef struct {
  char *data;
  char *null_bytemap; // for empty tables currently
  size_t data_size;
  SQL_Cartesian_Product_Layout *items;
  size_t size;
  size_t used;
} SQL_Unfiltered_Rowset;

typedef struct {
  SQL_Unfiltered_Rowset *items;
  size_t size;
  size_t used;
} SQL_Filtered_Rowset;

typedef struct {
  Word *items;
  size_t size;
  size_t used;
} SQL_Select_List;

typedef struct {
  char *data;
  char *null_bytemap; // for empty tables currently
} SQL_Result_Rowset;

typedef struct {
  SQL_Result_Rowset *items;
  SQL_Cartesian_Product_Layout *col_items;
  size_t col_size;
  size_t col_used;
  size_t data_size;
  size_t size;
  size_t used;
} SQL_Resultset;

#define sql_col_push(xs, x)                                                    \
  do {                                                                         \
    if (xs.col_used >= xs.col_size) {                                          \
      if (xs.col_size == 0)                                                    \
        xs.col_size = 4;                                                       \
      else                                                                     \
        xs.col_size *= 2;                                                      \
      xs.col_items =                                                           \
          realloc(xs.col_items, xs.col_size * sizeof(*xs.col_items));          \
    }                                                                          \
    xs.col_items[xs.col_used++] = x;                                           \
  } while (0)

#define sql_push(xs, x)                                                        \
  do {                                                                         \
    if (xs.used >= xs.size) {                                                  \
      if (xs.size == 0)                                                        \
        xs.size = 4;                                                           \
      else                                                                     \
        xs.size *= 2;                                                          \
      xs.items = realloc(xs.items, xs.size * sizeof(*xs.items));               \
    }                                                                          \
    xs.items[xs.used++] = x;                                                   \
  } while (0)

SQL_Program sql_program(Node *ast);
SQL_Program sql_gen_prog_from_ast_arr(Ast_Array ast);
void sql_exec_create(char *sql_text, VM *vm, MemArea *mem,
                     HashMap *table_definition);
void sql_exec_insert(char *sql_text, VM *vm, HashMap *table_definition,
                     MemArea *mem);
void sql_exec_select(char *sql_text, HashMap *table_definition, VM *vm,
                     MemArea *mem);
void sql_ins_buf_to_page(DB_Page *page, char *buf, int buf_size);
void sql_cartesian_rowset_generator(SQL_Table_List_n_Cursor *table_list,
                                    SQL_Unfiltered_Rowset **rowset,
                                    HashMap *table_definition, MemArea *mem);
int sql_get_table_tuplesize(char *tabname, HashMap *table_definition);
char *sql_get_table_tuple_by_slotno(char *tabname, int slotno, MemArea *mem,
                                    HashMap *table_definition);
int sql_get_table_field_offset_in_filtered_rowset(
    char *tabname, char *colname, SQL_Unfiltered_Rowset *rowset);
int sql_get_table_slotcnt(char *tabname, MemArea *mem);
SQL_Cartesian_Product_Layout sql_get_col_type_n_size(char *tabname,
                                                     char *colname,
                                                     HashMap *table_definition);
void sql_print_unfiltered_rowset(SQL_Unfiltered_Rowset *rowset);
void sql_print_resultset(SQL_Resultset *rowset);
int sql_is_in_table_list(SQL_Table_List table_list, char *tabname);
int sql_is_in_col_list(SQL_Col_Def_Array *col_def_arr, char *colname);
int sql_is_col_null(char *tabname, char *colname, SQL_Unfiltered_Rowset rowset);
void sql_exec_sql(char *sql, VM *vm, MemArea *mem, HashMap *table_definition);
