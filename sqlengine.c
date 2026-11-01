#include "sqlengine.h"
#include "hashmap.h"
#include "parse.h"
#include "sqldb.h"
#include "stackmachine.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

extern int global_pageno;

// in bytes
const int sql_type_len[] = {
    [SQL_TYPE_INT] = 8,
    [SQL_TYPE_FLOAT] = 8,
    [SQL_TYPE_CHAR] = 1,
};

static void preorder_tree(Node *node, Ast_Array *arr) {
  // make an array of only the headnode of a expression
  if (node->type.exacttype != COMMA && node->type.exacttype != ROOT &&
      node->type.exacttype != VALUES) { // TODO:temporary migigation that VALUES
                                        // in INSERT statement cause issue
    sql_push((*arr), *node);
    return;
  }
  for (int i = 0; i < node->childs->cnt; i++) {
    Node *cur_node = at(node->childs, i);
    preorder_tree(cur_node, arr);
  }
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
              .operand = {.as_ptr =
                              at(ast.items[0].childs, 1)
                                  ->token->str}})); // push tablename to stack;
      sql_push(prog, ((VM_Instruction){
                         .inst_type = INST_CREATE_TABLE,
                     }));
      for (int i = 1; i < ast.used; i++) {
        sql_push(
            prog,
            ((VM_Instruction){
                .inst_type = INST_PUSH,
                .operand = {
                    .as_ptr =
                        ast.items[i].token->str}})); // push colname to stack

        TokenType coltype = 0;
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "char") == 0) {
          coltype = STRING;
          sql_push(
              prog,
              ((VM_Instruction){
                  .inst_type = INST_PUSH,
                  .operand = {.as_i64 = coltype}})); // push coltype to stack
        }
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "integer") ==
            0) {
          coltype = INTEGER;
          sql_push(
              prog,
              ((VM_Instruction){
                  .inst_type = INST_PUSH,
                  .operand = {.as_i64 = coltype}})); // push coltype to stack
        }
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "float") == 0) {
          coltype = FLOAT;
          sql_push(
              prog,
              ((VM_Instruction){
                  .inst_type = INST_PUSH,
                  .operand = {.as_i64 = coltype}})); // push coltype to stack
        }

        //
        // push type length to stack according to type; any type except char is
        // 8 bytes;
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "char") != 0) {
          sql_push(prog, ((VM_Instruction){.inst_type = INST_PUSH,
                                           .operand = {.as_i64 = 8}}));
        } else {
          sql_push(
              prog,
              ((VM_Instruction){
                  .inst_type = INST_PUSH,
                  .operand = {.as_i64 = atoi(
                                  at(ast.items[i].childs, 1)->token->str)}}));
        }
        sql_push(prog, ((VM_Instruction){
                           .inst_type = INST_DEFINE_COLUMN,
                       }));
      }
    }
    break;

  case INSERT:

    sql_push(prog,
             ((VM_Instruction){
                 .inst_type = INST_PUSH,
                 .operand = {
                     .as_ptr = ast.items[0]
                                   .childs->items[0]
                                   .childs->items[0]
                                   .token->str}})); // push tablename to stack;
    sql_push(prog, ((VM_Instruction){
                       .inst_type = INST_INSERT_PREP,
                   }));

    for (int i = 1; i < ast.used; i++) {
      sql_push(prog, ((VM_Instruction){
                         .inst_type = INST_PUSH,
                         .operand = {.as_i64 = ast.items[i].token->type}}));
      if (ast.items[i].token->type == INTEGER) {
        sql_push(prog, ((VM_Instruction){
                           .inst_type = INST_PUSH,
                           .operand = {.as_i64 = ast.items[i].token->ival}}));
      }

      if (ast.items[i].token->type == FLOAT) {
        sql_push(prog, ((VM_Instruction){
                           .inst_type = INST_PUSH,
                           .operand = {.as_f64 = ast.items[i].token->fval}}));
      }
      if (ast.items[i].token->type == STRING) {
        sql_push(prog, ((VM_Instruction){
                           .inst_type = INST_PUSH,
                           .operand = {.as_ptr = ast.items[i].token->str}}));
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
  // for (int i = 0; i < arr_ast.used; i++) {
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

void sql_exec_select(char *sql_text, VM *vm, MemArea *mem,
                     HashMap *table_definition) {
  Token *token = tokenize(sql_text);
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

  DB_Page *thispage = vm->stack[0].as_ptr;       // extract page from stack;
  thispage->pageheader.pageno = global_pageno++; // set its pageno
  int idx = hashmap_append(&mem->pagedir, thispage->pageheader.objname);
  KV_append(mem->pagedir.items[idx], thispage->pageheader.pageno);

  SQL_Col_Def_Array *col_def_arr = vm->stack[1].as_ptr;
  mem_append(mem, *thispage);
  idx = hashmap_append(table_definition, thispage->pageheader.objname);
  KV_append(table_definition->items[idx], (uint64_t)col_def_arr);

  free(prog.items);
  free(col_def_arr->items);
  free(col_def_arr);
}

void sql_exec_insert(char *sql_text, VM *vm, HashMap table_definition,
                     MemArea *mem) {

  Token *token = tokenize(sql_text);
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
  char *ins_tabname = vm->stack[0].as_ptr;
  SQL_Insert_Val_Array *ins_val_arr = vm->stack[1].as_ptr;
  // insert values checked against table definition;
  int table_def_idx = hashmap_append(&table_definition, ins_tabname);
  SQL_Col_Def_Array *col_def_arr =
      (SQL_Col_Def_Array *)table_definition.items[table_def_idx].pageno;
  assert(col_def_arr->used == ins_val_arr->used &&
         "Insert values number mismatch with table definition");
  for (int i = 0; i < ins_val_arr->used; i++) {
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
  for (int i = 0; i < col_def_arr->used; i++) {
    tupesize += col_def_arr->items[i].len;
  }
  char buf[tupesize];
  memset(buf, 0, tupesize);
  int offset = 0;
  for (int i = 0; i < ins_val_arr->used; i++) {
    int len = col_def_arr->items[i].len;
    if (ins_val_arr->items[i].datatype == STRING) {
      memcpy(buf + offset, ins_val_arr->items[i].val.as_str,
             strlen(ins_val_arr->items[i].val.as_str));
    } else {
      memcpy(buf + offset, &ins_val_arr->items[i].val.as_int, len);
    }
    offset += len;
  }
  sql_ins_buf_to_page(mem->buffer + memloc, buf, tupesize);
  free(ins_val_arr->items);
  free(col_def_arr->items);
  free(ins_val_arr);
  free(col_def_arr);
}
