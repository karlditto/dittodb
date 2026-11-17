#include "sqlengine.h"
#include "hashmap.h"
#include "parse.h"
#include "sqldb.h"
#include "stackmachine.h"
#include <readline/chardefs.h>
#include <readline/readline.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <threads.h>
#include <time.h>

extern int global_pageno;

void sql_cartesian_rowset_generator(SQL_Table_List_n_Cursor *table_list,
                                    SQL_Unfiltered_Rowset **res_rowset,
                                    HashMap *table_definition, MemArea *mem) {

  // black magic
  static int state = 0;

  switch (state) {
  case 0:

    // execute once each generator call
    // for (int i = 0; i < table_list->used; i++) {
    //   printf("%s->%d ", table_list->items[i].node->token->str,
    //          table_list->items[i].slotcnt);
    // }
    // puts("");

    while (1) {

      // reset if got 2 NULL
      if (!table_list && !res_rowset) {
        state = 0;
        return;
      }
      state = 1;
      // generator code part:
      // initialize rowset
      int total_tuplesize = 0;
      int total_fieldcnt = 0;
      int fieldcnt[2] = {0};
      for (size_t i = 0; i < table_list->used; i++) {
        total_tuplesize += sql_get_table_tuplesize(
            table_list->items[i].node->token->str, table_definition);
        int table_def_idx = hashmap_find(table_definition,
                                         table_list->items[i].node->token->str);
        SQL_Col_Def_Array *col_def_arr =
            (SQL_Col_Def_Array *)table_definition->items[table_def_idx]
                .pageno[0];
        for (size_t j = 0; j < col_def_arr->used; j++) {
          fieldcnt[i]++;
          total_fieldcnt++;
          switch (col_def_arr->items[j].type) {
          case STRING:
            sql_push((**res_rowset),
                     ((SQL_Cartesian_Product_Layout){
                         .tabname = table_list->items[i].node->token->str,
                         .colname = col_def_arr->items[j].colname,
                         .type = SQL_TYPE_CHAR,
                         .len = col_def_arr->items[j].len}));
            break;
          case INTEGER:
            sql_push((**res_rowset),
                     ((SQL_Cartesian_Product_Layout){
                         .tabname = table_list->items[i].node->token->str,
                         .colname = col_def_arr->items[j].colname,
                         .type = SQL_TYPE_INT,
                         .len = col_def_arr->items[j].len}));
            break;
          case FLOAT:
            sql_push((**res_rowset),
                     ((SQL_Cartesian_Product_Layout){
                         .tabname = table_list->items[i].node->token->str,
                         .colname = col_def_arr->items[j].colname,
                         .type = SQL_TYPE_FLOAT,
                         .len = col_def_arr->items[j].len}));
            break;
          }
        }
      }

      (*res_rowset)->data_size = total_tuplesize;
      (*res_rowset)->data = calloc(1, total_tuplesize);
      (*res_rowset)->null_bytemap = calloc(total_fieldcnt, sizeof(char));

      // process the situation when there's only one table;
      if (table_list->used == 1) {
        if (table_list->items[0].cursor >= table_list->items[0].slotcnt) {
          free((*res_rowset)->data);
          free((*res_rowset)->null_bytemap);
          free((*res_rowset)->items);
          free(*res_rowset);
          *res_rowset = NULL;
          return;
        }
        //
        // printf("%s:%d\n", table_list->items[0].node->token->str,
        //        table_list->items[0].cursor);
        memcpy((*res_rowset)->data,
               sql_get_table_tuple_by_slotno(
                   table_list->items[0].node->token->str,
                   table_list->items[0].cursor, mem, table_definition),
               (*res_rowset)->data_size);
        // sql_print_unfiltered_rowset(*rowset);
        table_list->items[0].cursor++;
        //
        return;
      }

      // when there're 2 tables;
      char *t1 = table_list->items[0].node->token->str;
      char *t2 = table_list->items[1].node->token->str;
      int *t1cursor = &table_list->items[0].cursor;
      int t1slotcnt = table_list->items[0].slotcnt;
      int *t2cursor = &table_list->items[1].cursor;
      int t2slotcnt = table_list->items[1].slotcnt;
      int t1tuplesize = sql_get_table_tuplesize(t1, table_definition);
      int t2tuplesize = sql_get_table_tuplesize(t2, table_definition);

      if (t1slotcnt == 0) {
        if (*t2cursor >= t2slotcnt) {
          free((*res_rowset)->data);
          free((*res_rowset)->null_bytemap);
          free((*res_rowset)->items);
          free((*res_rowset));
          *res_rowset = NULL;
          return;
        }
        // printf("%s:%d %s:%d\n", t1, *t1cursor, t2, *t2cursor);
        memcpy((*res_rowset)->data,
               sql_get_table_tuple_by_slotno(
                   table_list->items[0].node->token->str,
                   table_list->items[0].cursor, mem, table_definition),
               t1tuplesize);
        memset((*res_rowset)->null_bytemap, 1, fieldcnt[0]);
        memcpy((*res_rowset)->data + t1tuplesize,
               sql_get_table_tuple_by_slotno(
                   table_list->items[1].node->token->str,
                   table_list->items[1].cursor, mem, table_definition),
               t2tuplesize);
        // sql_print_unfiltered_rowset(*rowset);
        *t2cursor += 1;
      } else if (t2slotcnt == 0) {
        if (*t1cursor >= t1slotcnt) {
          free((*res_rowset)->data);
          free((*res_rowset)->null_bytemap);
          free((*res_rowset)->items);
          free((*res_rowset));
          *res_rowset = NULL;
          return;
        }
        // printf("%s:%d %s:%d\n", t1, *t1cursor, t2, *t2cursor);
        memcpy((*res_rowset)->data,
               sql_get_table_tuple_by_slotno(
                   table_list->items[0].node->token->str,
                   table_list->items[0].cursor, mem, table_definition),
               t1tuplesize);
        memcpy((*res_rowset)->data + t1tuplesize,
               sql_get_table_tuple_by_slotno(
                   table_list->items[1].node->token->str,
                   table_list->items[1].cursor, mem, table_definition),
               t2tuplesize);
        memset((*res_rowset)->null_bytemap + fieldcnt[0], 1, fieldcnt[1]);
        // sql_print_unfiltered_rowset(*rowset);
        *t1cursor += 1;
      } else {
        if (*t1cursor >= t1slotcnt) {
          free((*res_rowset)->data);
          free((*res_rowset)->null_bytemap);
          free((*res_rowset)->items);
          free(*res_rowset);
          *res_rowset = NULL;
          return;
        }
        // printf("%s:%d %s:%d\n", t1, *t1cursor, t2, *t2cursor);
        memcpy((*res_rowset)->data,
               sql_get_table_tuple_by_slotno(
                   table_list->items[0].node->token->str,
                   table_list->items[0].cursor, mem, table_definition),
               t1tuplesize);
        memcpy((*res_rowset)->data + t1tuplesize,
               sql_get_table_tuple_by_slotno(
                   table_list->items[1].node->token->str,
                   table_list->items[1].cursor, mem, table_definition),
               t2tuplesize);
        // sql_print_unfiltered_rowset(*rowset);
        *t2cursor += 1;
        if (*t2cursor >= t2slotcnt) {
          *t2cursor = 0;
          *t1cursor += 1;
        }
      }

      //
      return;
    case 1:;
    }
  }
}
void sql_print_resultset(SQL_Resultset *rowset) {
  for (size_t i = 0; i < rowset->col_used; i++) {
    if (rowset->col_items[i].tabname == NULL)
      printf("%s", rowset->col_items[i].tabname);
    else
      printf("%s.%s", rowset->col_items[i].tabname,
             rowset->col_items[i].colname);
    if (i != rowset->col_used - 1) {
      printf(" || ");
    }
  }
  puts("");
  puts("=================================================================");
  for (size_t i = 0; i < rowset->used; i++) {
    int cursor = 0;
    for (size_t j = 0; j < rowset->col_used; j++) {
      if (rowset->items[i].null_bytemap[j]) {
        printf("(NULL)");
        cursor += rowset->col_items[j].len;
      } else {

        switch (rowset->col_items[j].type) {
        case SQL_TYPE_FLOAT: {
          double *f64 = (double *)(rowset->items[i].data + cursor);
          printf("%f", *f64);
          cursor += 8;
        } break;
        case SQL_TYPE_INT: {
          int64_t *i64 = (int64_t *)(rowset->items[i].data + cursor);
          printf("%ld", *i64);
          cursor += 8;
        } break;
        case SQL_TYPE_CHAR:
          printf("%.*s", (int)(rowset->col_items[j].len),
                 rowset->items[i].data + cursor);
          cursor += rowset->col_items[j].len;
          break;
        }
      }
      if (j != rowset->col_used - 1) {
        printf(" || ");
      }
    }

    puts("");
  }
}

void sql_print_unfiltered_rowset(SQL_Unfiltered_Rowset *rowset) {
  if (rowset == NULL) {
    return;
  }
  int cursor = 0;
  for (size_t i = 0; i < rowset->used; i++) {
    switch (rowset->items[i].type) {
    case SQL_TYPE_FLOAT:
      if (rowset->null_bytemap[i]) {
        printf("| (NULL) |, ");
        printf("  ");
        cursor += 8;
        continue;
      }
      double *f64 = (double *)(rowset->data + cursor);
      printf("|%-16f|, ", *f64);
      printf("  ");
      cursor += 8;
      break;
    case SQL_TYPE_INT:
      if (rowset->null_bytemap[i]) {
        printf("| (NULL) |, ");
        printf("  ");
        cursor += 8;
        continue;
      }
      int64_t *i64 = (int64_t *)(rowset->data + cursor);
      printf("|%-16ld|, ", *i64);
      printf("  ");
      cursor += 8;
      break;
    case SQL_TYPE_CHAR:
      if (rowset->null_bytemap[i]) {
        printf("| (NULL) |, ");
        printf("  ");
        cursor += rowset->items[i].len;
        continue;
      }
      printf("|%-*s|, ", (int)(rowset->items[i].len), rowset->data + cursor);
      printf("  ");
      cursor += rowset->items[i].len;
      break;
    }
  }
  puts("");
}

int sql_get_table_tuplesize(char *tabname, HashMap *table_definition) {
  int table_def_idx = hashmap_find(table_definition, tabname);
  SQL_Col_Def_Array *col_def_arr =
      (SQL_Col_Def_Array *)table_definition->items[table_def_idx].pageno[0];
  int tupesize = 0;
  for (size_t i = 0; i < col_def_arr->used; i++) {
    tupesize += col_def_arr->items[i].len;
  }
  return tupesize;
}

char *sql_get_table_tuple_by_slotno(char *tabname, int slotno, MemArea *mem,
                                    HashMap *table_definition) {
  int idx = hashmap_find(&mem->pagedir, tabname);
  int pageno = mem->pagedir.items[idx].pageno[0];
  char pageno_str[32];
  sprintf(pageno_str, "%d", pageno);
  idx = hashmap_find(&mem->pagetable, pageno_str);
  int memloc = mem->pagetable.items[idx].pageno[0];
  int tupesize = sql_get_table_tuplesize(tabname, table_definition);
  return &mem->buffer[memloc]
              .data[sizeof(mem->buffer[memloc].data) - tupesize * (slotno + 1)];
}

int sql_get_table_slotcnt(char *tabname, MemArea *mem) {
  int idx = hashmap_find(&mem->pagedir, tabname);
  int pageno = mem->pagedir.items[idx].pageno[0];
  char pageno_str[32];
  sprintf(pageno_str, "%d", pageno);
  idx = hashmap_find(&mem->pagetable, pageno_str);
  int memloc = mem->pagetable.items[idx].pageno[0];
  return mem->buffer[memloc].pageheader.slotno;
}

int sql_get_table_field_offset_in_filtered_rowset(
    char *tabname, char *colname, SQL_Unfiltered_Rowset *rowset) {
  int cursor = 0;
  for (size_t i = 0; i < rowset->used; i++) {
    if (strcasecmp(tabname, rowset->items[i].tabname) == 0 &&
        strcasecmp(colname, rowset->items[i].colname) == 0) {
      return cursor;
    }
    cursor += rowset->items[i].len;
  }
  return -1;
}

int sql_is_col_null(char *tabname, char *colname,
                    SQL_Unfiltered_Rowset rowset) {
  for (size_t i = 0; i < rowset.used; i++) {
    if (rowset.items[i].tabname == NULL || tabname == NULL) {
      continue;
    }
    if (strcasecmp(tabname, rowset.items[i].tabname) == 0 &&
        strcasecmp(colname, rowset.items[i].colname) == 0) {
      return rowset.null_bytemap[i];
    }
  }
  return 0;
}

SQL_Cartesian_Product_Layout
sql_get_col_type_n_size(char *tabname, char *colname,
                        HashMap *table_definition) {
  SQL_Cartesian_Product_Layout type_n_size = {0};
  int table_def_idx = hashmap_find(table_definition, tabname);
  SQL_Col_Def_Array *col_def_arr =
      (SQL_Col_Def_Array *)table_definition->items[table_def_idx].pageno[0];
  for (size_t i = 0; i < col_def_arr->used; i++) {
    if (strcasecmp(colname, col_def_arr->items[i].colname) == 0) {
      switch (col_def_arr->items[i].type) {
      case INTEGER:
        type_n_size.type = SQL_TYPE_INT;
        break;
      case STRING:
        type_n_size.type = SQL_TYPE_CHAR;
        break;
      case FLOAT:
        type_n_size.type = SQL_TYPE_FLOAT;
        break;
      default:
        type_n_size.type = SQL_TYPE_NOTYPE;
      }
      type_n_size.len = col_def_arr->items[i].len;
    }
  }
  return type_n_size;
}

static void preorder_tree(Node *node, Ast_Array *arr) {
  // make an array of only the headnode of a expression
  if (node->type.exacttype != COMMA && node->type.exacttype != ROOT &&
      node->type.exacttype != VALUES) { // TODO:temporary migigation that VALUES
    if (node->expr_rhs == NULL && node->expr_lhs != NULL &&
        (node->type.exacttype == SUB)) {
      // BUG: rhs and lhs of prefix should be reversed
      node->type.exacttype = SUB_PREFIX;
    }
    if (node->expr_rhs == NULL && node->expr_lhs != NULL &&
        (node->type.exacttype == ADD)) {
      // BUG: rhs and lhs of prefix should be reversed
      node->type.exacttype = ADD_PREFIX;
    } // in INSERT statement cause issue
    sql_push((*arr), *node);
    return;
  }
  for (size_t i = 0; i < node->childs->cnt; i++) {
    Node *cur_node = at(node->childs, i);

    preorder_tree(cur_node, arr);
  }
}
int sql_is_in_table_list(SQL_Table_List table_list, char *tabname) {
  for (size_t i = 0; i < table_list.used; i++) {
    if (strcasecmp(tabname, table_list.items[i]->token->str) == 0) {
      return 1;
    }
  }
  return 0;
}

int sql_is_in_col_list(SQL_Col_Def_Array *col_def_arr, char *colname) {
  for (size_t i = 0; i < col_def_arr->used; i++) {
    if (strcasecmp(colname, col_def_arr->items[i].colname) == 0) {
      return 1;
    }
  }
  return 0;
}

SQL_Program sql_gen_prog_from_ast_arr(Ast_Array ast) {
  SQL_Program prog = {0};
  switch (ast.items[0].type.exacttype) {
  case CREATE:

    if (at(ast.items[0].childs, 0)->type.exacttype == TABLE) {
      sql_push(
          prog,
          ((VM_Instruction){
              .inst_type = INST_PUSH,
              .operand = {.tag = WORD_PTR,
                          .val.as_ptr =
                              at(ast.items[0].childs, 1)
                                  ->token->str}})); // push tablename to stack;
      sql_push(prog, ((VM_Instruction){
                         .inst_type = INST_CREATE_TABLE,
                     }));
      for (size_t i = 1; i < ast.used; i++) {
        sql_push(
            prog,
            ((VM_Instruction){
                .inst_type = INST_PUSH,
                .operand = {
                    .tag = WORD_PTR,
                    .val.as_ptr =
                        ast.items[i].token->str}})); // push colname to stack

        TokenType coltype = {0};
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "char") == 0) {
          coltype = STRING;
          sql_push(prog,
                   ((VM_Instruction){
                       .inst_type = INST_PUSH,
                       .operand = {.tag = WORD_I64,
                                   .val.as_i64 =
                                       coltype}})); // push coltype to stack
        }
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "integer") ==
            0) {
          coltype = INTEGER;
          sql_push(prog,
                   ((VM_Instruction){
                       .inst_type = INST_PUSH,
                       .operand = {.tag = WORD_I64,
                                   .val.as_i64 =
                                       coltype}})); // push coltype to stack
        }
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "float") == 0) {
          coltype = FLOAT;
          sql_push(prog,
                   ((VM_Instruction){
                       .inst_type = INST_PUSH,
                       .operand = {.tag = WORD_I64,
                                   .val.as_i64 =
                                       coltype}})); // push coltype to stack
        }

        //
        // push type length to stack according to type; any type except char is
        // 8 bytes;
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "char") != 0) {
          sql_push(prog, ((VM_Instruction){
                             .inst_type = INST_PUSH,
                             .operand = {.tag = WORD_I64, .val.as_i64 = 8}}));
        } else {
          sql_push(
              prog,
              ((VM_Instruction){
                  .inst_type = INST_PUSH,
                  .operand = {.tag = WORD_I64,
                              .val.as_i64 = atoi(
                                  at(ast.items[i].childs, 1)->token->str)}}));
        }
        sql_push(prog, ((VM_Instruction){
                           .inst_type = INST_DEFINE_COLUMN,
                       }));
      }
    }
    break;

  case INSERT:

    sql_push(
        prog,
        ((VM_Instruction){
            .inst_type = INST_PUSH,
            .operand = {.tag = WORD_PTR,
                        .val.as_ptr =
                            ast.items[0]
                                .childs->items[0]
                                .childs->items[0]
                                .token->str}})); // push tablename to stack;
    sql_push(prog, ((VM_Instruction){
                       .inst_type = INST_INSERT_PREP,
                   }));

    for (size_t i = 1; i < ast.used; i++) {
      sql_push(prog, ((VM_Instruction){
                         .inst_type = INST_PUSH,
                         .operand = {.tag = WORD_I64,
                                     .val.as_i64 = ast.items[i].token->type}}));
      if (ast.items[i].token->type == INTEGER) {
        sql_push(prog,
                 ((VM_Instruction){
                     .inst_type = INST_PUSH,
                     .operand = {.tag = WORD_I64,
                                 .val.as_i64 = ast.items[i].token->ival}}));
      }

      if (ast.items[i].token->type == FLOAT) {
        sql_push(prog,
                 ((VM_Instruction){
                     .inst_type = INST_PUSH,
                     .operand = {.tag = WORD_F64,
                                 .val.as_f64 = ast.items[i].token->fval}}));
      }
      if (ast.items[i].token->type == STRING) {
        sql_push(prog,
                 ((VM_Instruction){
                     .inst_type = INST_PUSH,
                     .operand = {.tag = WORD_PTR,
                                 .val.as_ptr = ast.items[i].token->str}}));
      }
      sql_push(prog, ((VM_Instruction){
                         .inst_type = INST_INSERT_VALUE,
                     }));
    }
    break;
  }
  sql_push(prog, ((VM_Instruction){.inst_type = INST_HLT}));

  free(ast.items);
  return prog;
}

SQL_Program sql_program(Node *ast) {
  SQL_Program program = {0};
  Ast_Array arr_ast = {0};
  preorder_tree(ast, &arr_ast);
  // for (size_t i = 0; i < arr_ast.used; i++) {
  //   print_ast(&arr_ast.items[i], 0);
  //   puts("");
  // }
  return sql_gen_prog_from_ast_arr(arr_ast);
}

void sql_ins_buf_to_page(DB_Page *page, char *buf, int buf_size) {
  memcpy(page->data + page->pageheader.freespacetr - buf_size + 1, buf,
         buf_size);
  page->pageheader.freespace -= buf_size;
  page->pageheader.freespacetr -= buf_size;
  page->pageheader.slotno += 1;
}

void sql_exec_create(char *sql_text, VM *vm, MemArea *mem,
                     HashMap *table_definition) {
  Token *token = tokenize(sql_text);
  // print_token(token);
  Node *ast = parse(token);
  SQL_Program prog = sql_program(ast);
  // vm_dump_inst(prog.items, prog.used);
  //
  while (!vm->halt) {
    // vm_dump_stack_n_inst(vm, prog.items);
    VM_Instruction_Signal sig = vm_exec_prog(vm, prog.items, prog.used);
    if (sig != INST_SIG_OK) {
      printf("%s\n", vm_sig_as_cstr(sig));
      // vm_dump_stack_n_inst(vm, prog.items);
      exit(1);
    }
  }

  DB_Page *thispage = vm->stack[0].val.as_ptr;   // extract page from stack;
  thispage->pageheader.pageno = global_pageno++; // set its pageno
  int idx = hashmap_append(&mem->pagedir, thispage->pageheader.objname);
  KV_append(mem->pagedir.items[idx], thispage->pageheader.pageno);

  SQL_Col_Def_Array *col_def_arr = vm->stack[1].val.as_ptr;
  mem_append(mem, *thispage);
  idx = hashmap_append(table_definition, thispage->pageheader.objname);
  KV_append(table_definition->items[idx], (uint64_t)col_def_arr);

  free(prog.items);
  free(thispage);
}

void sql_exec_insert(char *sql_text, VM *vm, HashMap *table_definition,
                     MemArea *mem) {

  Token *token = tokenize(sql_text);
  // print_token(token);
  Node *ast = parse(token);
  SQL_Program prog = sql_program(ast);
  // vm_dump_inst(prog.items, prog.used);
  while (!vm->halt) {
    // vm_dump_stack_n_inst(vm, prog.items);
    VM_Instruction_Signal sig = vm_exec_prog(vm, prog.items, prog.used);
    if (sig != INST_SIG_OK) {
      printf("%s\n", vm_sig_as_cstr(sig));
      // vm_dump_stack_n_inst(vm, prog.items);
      exit(1);
    }
  }
  char *ins_tabname = vm->stack[0].val.as_ptr;
  SQL_Insert_Val_Array *ins_val_arr = vm->stack[1].val.as_ptr;
  // insert values checked against table definition;
  int table_def_idx = hashmap_find(table_definition, ins_tabname);
  SQL_Col_Def_Array *col_def_arr =
      (SQL_Col_Def_Array *)table_definition->items[table_def_idx].pageno[0];
  assert(col_def_arr->used == ins_val_arr->used &&
         "Insert values number mismatch with table definition");
  for (size_t i = 0; i < ins_val_arr->used; i++) {
    assert(col_def_arr->items[i].type == ins_val_arr->items[i].datatype &&
           "Insert values type mismatch with table definition");
  }

  int idx = hashmap_find(&mem->pagedir, ins_tabname);
  int pageno = mem->pagedir.items[idx].pageno[0];
  char pageno_str[32];
  sprintf(pageno_str, "%d", pageno);
  idx = hashmap_find(&mem->pagetable, pageno_str);
  int memloc = mem->pagetable.items[idx].pageno[0];

  // calculate tupesize
  int tupesize = 0;
  for (size_t i = 0; i < col_def_arr->used; i++) {
    tupesize += col_def_arr->items[i].len;
  }
  char buf[tupesize];
  memset(buf, 0, tupesize);
  int offset = 0;
  for (size_t i = 0; i < ins_val_arr->used; i++) {
    int len = col_def_arr->items[i].len;
    if (ins_val_arr->items[i].datatype == STRING) {
      memcpy(buf + offset, ins_val_arr->items[i].val.as_str, len - 1);
    } else {
      memcpy(buf + offset, &ins_val_arr->items[i].val.as_int, len);
    }
    offset += len;
  }
  sql_ins_buf_to_page(mem->buffer + memloc, buf, tupesize);

  free(prog.items);
  free(ins_val_arr->items);
  free(ins_val_arr);
}

static void sql_from_postorder(Node *from_node, SQL_Table_List *table_list) {
  if (from_node == NULL)
    return;
  if (from_node->type.exacttype != COMMA) {
    sql_push((*table_list), from_node);
    return;
  };
  for (size_t i = 0; i < from_node->childs->cnt; i++) {
    sql_from_postorder(&from_node->childs->items[i], table_list);
  }
}
static void sql_select_postorder(Node *select_node, Ast_Array *arr) {
  if (select_node == NULL)
    return;
  if (select_node->type.exacttype == DOT) {
    sql_push((*arr), *select_node);
    return;
  }
  sql_select_postorder(select_node->expr_lhs, arr);
  sql_select_postorder(select_node->expr_rhs, arr);
  if (select_node->type.exacttype == COMMA) {
    return;
  }
  sql_push((*arr), *select_node);
}

static void sql_where_postorder(Node *where_node, Ast_Array *RPN) {
  // should skip the where keyword node
  if (where_node == NULL) {
    return;
  }
  if (where_node->expr_rhs == NULL && where_node->expr_lhs != NULL &&
      (where_node->type.exacttype == SUB)) {
    // BUG: rhs and lhs of prefix should be reversed
    where_node->type.exacttype = SUB_PREFIX;
  }
  if (where_node->expr_rhs == NULL && where_node->expr_lhs != NULL &&
      (where_node->type.exacttype == ADD)) {
    // BUG: rhs and lhs of prefix should be reversed
    where_node->type.exacttype = ADD_PREFIX;
  }
  sql_where_postorder(where_node->expr_lhs, RPN);
  sql_where_postorder(where_node->expr_rhs, RPN);
  sql_push((*RPN), *where_node);
}

void sql_exec_select(char *sql_text, HashMap *table_definition, VM *vm,
                     MemArea *mem) {
  /* NOTE: select exec includes 3 parts:
   * 1: cartesian product of tables
   * 2: filtering
   * 3: projection
   */
  Token *token = tokenize(sql_text);
  // print_token(token);
  Node *ast = parse(token);
  print_ast_tree(ast, 0);
  SQL_Program prog = {0};

  // process from
  SQL_Table_List table_list = {0};
  Node *from_node = {0};
  for (size_t i = 0; i < ast->childs->cnt; i++) {
    if (ast->childs->items[i].type.exacttype == FROM) {
      from_node = &ast->childs->items[i].childs->items[0];
    }
  }
  sql_from_postorder(from_node, &table_list);
  // NOTE: currently not allow joining more than 2 tables;
  if (table_list.used > 2) {
    printf("[ERROR] currently not allow joining more than 2 tables\n");
    free(table_list.items);
    return;
  }
  for (size_t i = 0; i < table_list.used; i++) {
    if (hashmap_find(table_definition, table_list.items[i]->token->str) < 0) {
      printf("[ERROR] exec select failed, table dose not exist: %s\n",
             table_list.items[i]->token->str);
      free(table_list.items);
      return;
    }
    sql_push(prog, vm_push((Word){.tag = WORD_PTR,
                                  .val.as_ptr = table_list.items[i]}));
  }
  sql_push(prog, vm_push((Word){.tag = WORD_PTR, .val.as_ptr = mem}));
  sql_push(prog, vm_sql_from_table());
  sql_push(prog,
           vm_push((Word){
               .tag = WORD_PTR,
               .val.as_ptr =
                   mem})); // need to push again because last inst consumes it
  sql_push(prog,
           vm_push((Word){.tag = WORD_PTR, .val.as_ptr = table_definition}));
  SQL_Unfiltered_Rowset *unfiltered_rowset = NULL;
  // push this to stack for
  // passing malloced pointer
  SQL_Filtered_Rowset *filtered_rowset = calloc(1, sizeof(SQL_Filtered_Rowset));
  sql_push(prog,
           vm_push((Word){.tag = WORD_PTR, .val.as_ptr = filtered_rowset}));
  sql_push(prog,
           vm_push((Word){.tag = WORD_PTR, .val.as_ptr = unfiltered_rowset}));
  sql_push(prog, vm_gen());
  int gen_iidx = prog.used - 1;
  sql_push(prog, vm_hlt_ifz());

  // process where
  Node *where_expr_node = {0};
  for (size_t i = 0; i < ast->childs->cnt; i++) {
    if (ast->childs->items[i].type.exacttype == WHERE) {
      // skip WHERE keyword node
      where_expr_node = &ast->childs->items[i].childs->items[0];
    }
  }
  /* NOTE: TOP values on the stack after INST_FROM_TABLES:
   * 0: SQL_Table_List_n_Cursor *tab_list
   * 1: (void *)sql_cartesian_rowset_generator()
   * 2: MemArea *
   * 3: HashMap *table_definition
   * 4: SQL_Filtered_Rowset *
   * 5: SQL_Unfiltered_Rowset *
   */
  Ast_Array RPN = {0};
  sql_where_postorder(where_expr_node, &RPN);
  for (size_t i = 0; i < RPN.used; i++) {
    switch (RPN.items[i].type.exacttype) {
    case ADD:
      sql_push(prog, vm_add());
      break;
    case SUB:
      sql_push(prog, vm_sub());
      break;
    case ADD_PREFIX:
      sql_push(prog, vm_add_prefix());
      break;
    case SUB_PREFIX:
      sql_push(prog, vm_sub_prefix());
      break;
    case MUL:
      sql_push(prog, vm_mul());
      break;
    case DIV:
      sql_push(prog, vm_div());
      break;
    case AND:
      sql_push(prog, vm_and());
      break;
    case OR:
      sql_push(prog, vm_or());
      break;
    case EQ:
      sql_push(prog, vm_eq());
      break;
    case NEQ:
      sql_push(prog, vm_neq());
      break;
    case LT:
      sql_push(prog, vm_lt());
      break;
    case LEQ:
      sql_push(prog, vm_leq());
      break;
    case GT:
      sql_push(prog, vm_gt());
      break;
    case GEQ:
      sql_push(prog, vm_geq());
      break;
    case ATOM:
      switch (RPN.items[i].token->type) {
      case STRING:
        sql_push(prog, vm_push((Word){.tag = WORD_STR,
                                      .val.as_ptr = RPN.items[i].token->str}));
        break;
      case INTEGER:
        sql_push(prog, vm_push((Word){.tag = WORD_I64,
                                      .val.as_i64 = RPN.items[i].token->ival}));

        break;
      case FLOAT:
        sql_push(prog, vm_push((Word){.tag = WORD_F64,
                                      .val.as_f64 = RPN.items[i].token->fval}));

        break;
      }
      break;

    case DOT:
      if (RPN.items[i].expr_lhs == NULL) {
        printf("[ERROR]: tabname identifier missing\n");
        free(table_list.items);
        free(RPN.items);
        return;
      }
      if (RPN.items[i].expr_rhs == NULL) {
        printf("[ERROR]: colname identifier missing\n");
        free(table_list.items);
        free(RPN.items);
        return;
      }
      if (!sql_is_in_table_list(table_list,
                                RPN.items[i].expr_lhs->token->str)) {
        printf("[ERROR]: this table: \"%s\" not in table list\n",
               RPN.items[i].expr_lhs->token->str);
        free(table_list.items);
        free(RPN.items);
        return;
      }
      int idx =
          hashmap_find(table_definition, RPN.items[i].expr_lhs->token->str);
      if (idx < 0) {
        printf("[ERROR]: tabname \"%s\" identifier missing\n",
               RPN.items[i].token->str);
        free(table_list.items);
        free(RPN.items);
        return;
      }
      if (!sql_is_in_col_list(
              (SQL_Col_Def_Array *)table_definition->items[idx].pageno[0],
              RPN.items[i].expr_rhs->token->str)) {
        printf("[ERROR]: this column: \"%s\" not in column list\n",
               RPN.items[i].expr_rhs->token->str);
        free(table_list.items);
        free(RPN.items);
        return;
      }
      sql_push(prog,
               vm_push((Word){.tag = WORD_PTR, .val.as_ptr = &RPN.items[i]}));
      sql_push(prog, vm_fetchval());
      break;
    }
  }
  if (RPN.used == 0) {
    sql_push(prog, vm_push((Word){
                       .tag = WORD_I64,
                       .val.as_i64 = 1,
                   }));
  }

  sql_push(prog, vm_push_res_nz());
  sql_push(prog, vm_jmp(gen_iidx));
  sql_push(prog, vm_hlt());
  vm_dump_inst(prog.items, prog.used);
  while (!vm->halt) {
    vm_dump_stack_n_inst(*vm, prog.items);
    VM_Instruction_Signal sig = vm_exec_prog(vm, prog.items, prog.used);
    if (sig != INST_SIG_OK) {
      printf("%s\n", vm_sig_as_cstr(sig));
      vm_dump_stack_n_inst(*vm, prog.items);
      exit(1);
    }
  }
  // vm->stack_size--; // pop the (nil)
  // vm_dump_stack(*vm);

  // refresh program and vm
  SQL_Unfiltered_Rowset *rowset = vm->stack[5].val.as_ptr;
  free(rowset);
  vm->stack_size--; // pop the (nil)
  prog.size = 0;
  prog.used = 0;
  vm->pinst = 0;
  vm->halt = 0;

  // process select
  Node *select_node = {0};
  for (size_t i = 0; i < ast->childs->cnt; i++) {
    if (ast->childs->items[i].type.exacttype == SELECT) {
      // skip WHERE keyword node
      select_node = &ast->childs->items[i].childs->items[0];
    }
  }
  SQL_Select_List *select_list = calloc(1, sizeof(SQL_Select_List));
  sql_push(prog, vm_push((Word){
                     .tag = WORD_PTR,
                     .val.as_ptr = select_list,
                 }));

  Ast_Array select_separated = {0};
  Ast_Array select_list_arr_RPN = {0};
  preorder_tree(select_node, &select_separated);
  for (size_t i = 0; i < select_separated.used; i++) {
    sql_select_postorder(&select_separated.items[i], &select_list_arr_RPN);
    sql_push(select_list_arr_RPN, ((Node){.type.exacttype = COMMA}));
  }
  for (size_t i = 0; i < select_list_arr_RPN.used; i++) {
    // printf("%s\n", select_list_arr_RPN.items[i].token->str);
    switch (select_list_arr_RPN.items[i].type.exacttype) {
    case ADD:
      sql_push(prog, vm_add());
      break;
    case SUB:
      sql_push(prog, vm_sub());
      break;
    case ADD_PREFIX:
      sql_push(prog, vm_add_prefix());
      break;
    case SUB_PREFIX:
      sql_push(prog, vm_sub_prefix());
      break;
    case MUL:
      sql_push(prog, vm_mul());
      break;
    case DIV:
      sql_push(prog, vm_div());
      break;
    case AND:
      sql_push(prog, vm_and());
      break;
    case OR:
      sql_push(prog, vm_or());
      break;
    case EQ:
      sql_push(prog, vm_eq());
      break;
    case NEQ:
      sql_push(prog, vm_neq());
      break;
    case LT:
      sql_push(prog, vm_lt());
      break;
    case LEQ:
      sql_push(prog, vm_leq());
      break;
    case GT:
      sql_push(prog, vm_gt());
      break;
    case GEQ:
      sql_push(prog, vm_geq());
      break;
    case COMMA:
      sql_push(prog, vm_push_sel_list());
      break;
    case ATOM:
      switch (select_list_arr_RPN.items[i].token->type) {
      case STRING:
        sql_push(prog,
                 vm_push((Word){.tag = WORD_STR,
                                .val.as_ptr =
                                    select_list_arr_RPN.items[i].token->str}));
        break;
      case INTEGER:
        sql_push(prog,
                 vm_push((Word){.tag = WORD_I64,
                                .val.as_i64 =
                                    select_list_arr_RPN.items[i].token->ival}));
        break;
      case FLOAT:
        sql_push(prog,
                 vm_push((Word){.tag = WORD_F64,
                                .val.as_f64 =
                                    select_list_arr_RPN.items[i].token->fval}));
        break;
      }
      break;

    case DOT:
      if (select_list_arr_RPN.items[i].expr_lhs == NULL) {
        printf("[ERROR]: tabname identifier missing\n");
        free(table_list.items);
        free(RPN.items);
        free(select_separated.items);
        free(select_list_arr_RPN.items);
        return;
      }
      if (select_list_arr_RPN.items[i].expr_rhs == NULL) {
        printf("[ERROR]: colname identifier missing\n");
        free(table_list.items);
        free(RPN.items);
        free(select_separated.items);
        free(select_list_arr_RPN.items);
        return;
      }
      if (!sql_is_in_table_list(
              table_list, select_list_arr_RPN.items[i].expr_lhs->token->str)) {
        printf("[ERROR]: this table: \"%s\" not in table list\n",
               select_list_arr_RPN.items[i].expr_lhs->token->str);
        free(table_list.items);
        free(RPN.items);
        free(select_separated.items);
        free(select_list_arr_RPN.items);
        return;
      }
      int idx = hashmap_find(table_definition,
                             select_list_arr_RPN.items[i].expr_lhs->token->str);
      if (idx < 0) {
        printf("[ERROR]: tabname \"%s\" identifier missing\n",
               select_list_arr_RPN.items[i].token->str);
        free(table_list.items);
        free(RPN.items);
        free(select_separated.items);
        free(select_list_arr_RPN.items);
        return;
      }
      if (!sql_is_in_col_list(
              (SQL_Col_Def_Array *)table_definition->items[idx].pageno[0],
              select_list_arr_RPN.items[i].expr_rhs->token->str) &&
          select_list_arr_RPN.items[i].expr_rhs->type.exacttype != MUL) {
        printf("[ERROR]: this column: \"%s\" not in column list\n",
               select_list_arr_RPN.items[i].expr_rhs->token->str);
        free(table_list.items);
        free(RPN.items);
        free(select_separated.items);
        free(select_list_arr_RPN.items);
        return;
      }
      sql_push(prog,
               vm_push((Word){.tag = WORD_PTR,
                              .val.as_ptr = &select_list_arr_RPN.items[i]}));
      break;
    }
  }
  SQL_Resultset *resset = calloc(1, sizeof(SQL_Resultset));
  sql_push(prog, vm_push((Word){.tag = WORD_PTR, .val.as_ptr = resset}));
  sql_push(prog, vm_select_project());
  sql_push(prog, vm_hlt());
  // vm_dump_inst(prog.items, prog.used);
  while (!vm->halt) {
    // vm_dump_stack_n_inst(*vm, prog.items);
    VM_Instruction_Signal sig = vm_exec_prog(vm, prog.items, prog.used);
    if (sig != INST_SIG_OK) {
      printf("%s\n", vm_sig_as_cstr(sig));
      vm_dump_stack_n_inst(*vm, prog.items);
      exit(1);
    }
  }
  // vm_dump_stack(*vm);
  /* NOTE: TOP values on the stack after INST_FROM_TABLES:
   * 0: SQL_Table_List_n_Cursor *tab_list
   * 1: (void *)sql_cartesian_rowset_generator()
   * 2: MemArea *
   * 3: HashMap *table_definition
   * 4: SQL_Filtered_Rowset *
   * 5: SQL_Select_List *
   * 6: SQL_Resultset *
   */
  free(select_separated.items);
  free(prog.items);
  free(select_list_arr_RPN.items);
  free(RPN.items);
  free(table_list.items);
  SQL_Filtered_Rowset *res = vm->stack[4].val.as_ptr;
  for (size_t i = 0; i < res->used; i++) {
    free(res->items[i].data);
    free(res->items[i].items);
    free(res->items[i].null_bytemap);
  }
  free(res->items);
  free(res);
  SQL_Table_List_n_Cursor *tab_list = vm->stack[0].val.as_ptr;
  free(tab_list->items);
  free(tab_list);

  SQL_Select_List *sel_lis = vm->stack[5].val.as_ptr;
  free(sel_lis->items);
  free(sel_lis);

  SQL_Resultset *resset_tmp = vm->stack[6].val.as_ptr;
  for (size_t i = 0; i < resset_tmp->used; i++) {
    free(resset_tmp->items[i].data);
    free(resset_tmp->items[i].null_bytemap);
  }
  free(resset_tmp->items);
  free(resset_tmp->col_items);
  free(resset_tmp);
}

void sql_exec_sql(char *sql, VM *vm, MemArea *mem, HashMap *table_definition) {
  Token *token = tokenize(sql);
  Node *ast = parse(token);
  // print_ast_tree(ast, 1);
  if (ast == NULL) {
    return;
  }
  // print_ast_tree(ast, 1);
  switch (ast->childs->items[0].type.exacttype) {
  case CREATE:
    sql_exec_create(sql, vm, mem, table_definition);
    vm->stack_size = 0, vm->halt = 0, vm->pinst = 0;
    break;
  case INSERT:
    sql_exec_insert(sql, vm, table_definition, mem);
    vm->stack_size = 0, vm->halt = 0, vm->pinst = 0;
    break;
  case FROM:
  case SELECT:
    sql_exec_select(sql, table_definition, vm, mem);
    vm->stack_size = 0, vm->halt = 0, vm->pinst = 0;
    vm->stack_size = 0, vm->halt = 0, vm->pinst = 0;
    break;
  }
}
