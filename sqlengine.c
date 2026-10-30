#include "parse.h"
#include "sqldb.h"
#include "stackmachine.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

// in bytes
const int sql_type_len[] = {
    [SQL_TYPE_INT] = 8,
    [SQL_TYPE_FLOAT] = 8,
    [SQL_TYPE_CHAR] = 1,
};

static void preorder_tree(Node *node, Ast_Array *arr) {
  // make an array of only the headnode of a expression
  if (node->type.exacttype != COMMA && node->type.exacttype != ROOT) {
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
    // first pass, calculate tuple size
    if (at(ast.items[0].childs, 0)->type.exacttype == TABLE) {
      int64_t tuplesize = 0;
      for (int i = 1; i < ast.used; i++) {
        if (strcasecmp(at(ast.items[i].childs, 0)->token->str, "char") != 0) {
          tuplesize += 8;
        } else {
          tuplesize += atoi(at(ast.items[i].childs, 1)->token->str);
        }
      }
      sql_push(
          prog,
          ((VM_Instruction){
              .inst_type = INST_PUSH,
              .operand = {.as_ptr = at(ast.items[0].childs, 1)->token->str}}));
      sql_push(prog, ((VM_Instruction){.inst_type = INST_PUSH,
                                       .operand = tuplesize}));
      sql_push(prog, ((VM_Instruction){
                         .inst_type = INST_CREATE_TABLE,
                     }));
    }
    break;
  }
  sql_push(prog, ((VM_Instruction){.inst_type = INST_HLT}));
  return prog;
}

SQL_Program sql_program(Node *ast) {
  SQL_Program program = {0};
  Ast_Array arr_ast = {0};
  preorder_tree(ast, &arr_ast);
  for (int i = 0; i < arr_ast.used; i++) {
    print_ast(&arr_ast.items[i], 0);
    puts("");
  }
  return sql_gen_prog_from_ast_arr(arr_ast);
}
