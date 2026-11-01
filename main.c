#include "hashmap.h"
#include "parse.h"
#include "sqldb.h"
#include "sqlengine.h"
#include "stackmachine.h"
#include <assert.h>
#include <fcntl.h>
#include <iso646.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define ARENA_SIZE_BYTES 20 * 1024 * 1024

Region mem_ast;

int main_sqlparser(int argc, char *argv[]) {

  if (argc > 2) {
    fprintf(stderr, "too many arguments.\n");
    exit(EXIT_FAILURE);
  }

  if (argc == 2) {
    mem_ast = region_init(ARENA_SIZE_BYTES);
    Token *token = tokenize(argv[1]);
    print_token(token);

    Node *ast = parse(token);
    print_ast(ast, 1);
    // token_free(token);
    printf("%lu\n", mem_ast.used);
    region_free(&mem_ast);
    return 0;
  }

  char *command = NULL;
  char *prompt = "SQL> ";
  while (true) {
    mem_ast = region_init(ARENA_SIZE_BYTES);
    command = readline(prompt);
    if (!command) {
      break;
    }
    if (command && *command)
      add_history(command);
    Token *token = tokenize(command);
    free(command);
    print_token(token);

    Node *ast = parse(token);
    print_ast(ast, 1);
    // token_free(token);
    region_free(&mem_ast);
  }
  free(mem_ast.data);
  free(command);

  return 0;
}

int cmp(const void *a, const void *b) {
  const KV *akv = a;
  const KV *bkv = b;
  return bkv->cnt - akv->cnt;
}

int main_hashtable() {
  HashMap ht = {0};
  hashmap_init(&ht, 2);

  int fd = open("t8.shakespeare.txt", O_RDONLY);
  assert(fd > 0);
  struct stat *statbuf = malloc(sizeof(struct stat));
  assert(fstat(fd, statbuf) == 0);
  char buf[statbuf->st_size + 1];
  read(fd, buf, statbuf->st_size);
  close(fd);

  // read word by word from txt file
  char *s = buf;
  unsigned long word_cnt = 0;
  char word_buf[33] = {0};
  int flag = 0;
  while (*s) {
    // if (ht.cnt >= 16) {
    //   break;
    // }

    if (isspace(*s) != 0 || *s == ',' || *s == '.' || *s == ';' || *s == ':' ||
        *s == '?' || *s == '!') {
      s++;
      continue;
    } else {
      char *s2 = s;
      while (true) {
        if (isspace(*s) != 0 || *s == ',' || *s == '.' || *s == ';' ||
            *s == ':' || *s == '?' || *s == '!') {
          int len = s - s2;
          memset(word_buf, 0, sizeof(word_buf));
          sprintf(word_buf, "%.*s", len, s2);
          // printf("%s\n", word_buf);
          // printf("%lu ", word_cnt);
          // printf("0x%08lX: ", hash(word_buf, strlen(word_buf)) % hashbucket);
          // printf("%lu: ", hash(word_buf, strlen(word_buf)) % hashbucket);
          // printf("%.*s\n", len, s2);

          if (hashmap_find(&ht, word_buf) != -1) {
            int64_t found = hashmap_find(&ht, word_buf);
            ht.items[found].cnt++;
            word_cnt++;
            break;
          }

          hashmap_append(&ht, word_buf);
          int64_t insert = hashmap_find(&ht, word_buf);
          ht.items[insert].cnt++;
          word_cnt++;
          if (flag == 0) {
            if (hashmap_find(&ht, "the") != -1) {
              hashmap_delete(&ht, "the");
              flag = 1;
            }
          }
          break;
        }
        s++;
      }
      continue;
    }
  }

  // hashmap_print(&ht);
  //
  KV *sorted = malloc(sizeof(KV) * ht.capa);
  memcpy(sorted, ht.items, sizeof(KV) * ht.capa);
  qsort(sorted, ht.capa, sizeof(KV), cmp);

  for (size_t i = 0; i < ht.capa; i++) {
    printf("key:%-20s --> cnt:%ld\n", sorted[i].objname, sorted[i].cnt);
  }

  free(sorted);
  hashmap_free(&ht);

  return 0;
}

#include "pokemon.h"

int main_vm() {
  VM vm = {0};
  // VM_Instruction fib[] = {vm_push(0), vm_push(1), vm_dup(1),
  //                             vm_dup(1),  vm_add(),   vm_jmp(2)};

  // calculate pi/4 with taylor series of arctan (very slow)
  // VM_Instruction program[] = {vm_push((Word){.as_f64 = -1}),
  //                             vm_push((Word){.as_f64 = 1}),
  //                             vm_push((Word){.as_f64 = 1}),
  //                             vm_dup(2),
  //                             vm_dup(2),
  //                             vm_push((Word){.as_f64 = 2}),
  //                             vm_fadd(),
  //                             vm_dup(1),
  //                             vm_dup(1),
  //                             vm_fdiv(),
  //                             vm_dup(3),
  //                             vm_fadd(),
  //                             vm_dup(2),
  //                             vm_dup(2),
  //                             vm_push((Word){.as_f64 = 2}),
  //                             vm_fadd(),
  //                             vm_dup(1),
  //                             vm_dup(1),
  //                             vm_fdiv(),
  //                             vm_dup(3),
  //                             vm_fsub(),
  //                             vm_push((Word){.as_f64 = -1}),
  //                             vm_fmul(),
  //                             vm_jmp(3)};
  //

  VM_Instruction program[] = {
      vm_push((Word){.as_i64 = 32}),
      vm_push((Word){.as_i64 = 33}),
      vm_swap(0),
      vm_hlt(),
  };
  while (!vm.halt) {

    vm_dump_stack_n_inst(vm, program);
    VM_Instruction_Signal sig = vm_exec_prog(&vm, program, ARRAY_SIZE(program));
    if (sig != INST_SIG_OK) {
      printf("%s\n", vm_sig_as_cstr(sig));
      vm_dump_stack_n_inst(vm, program);
      exit(1);
    }
    fgetc(stdin);
  }
  return 0;
}

int global_pageno = 0;
#define str(x) #x
int main(int argc, char **argv) {
  MemArea mem = {0};
  init_mem(mem);

  HashMap table_definition = {0};
  hashmap_init(&table_definition, 64);
  mem_ast = region_init(ARENA_SIZE_BYTES);
  VM vm = {0};

  // TEST: TEST CREATE
  Token *token =
      tokenize("create table t1 (id integer, txt char(16), remark char(32))");

  Node *ast = parse(token);
  // print_ast_tree(ast, 1);
  SQL_Program prog = sql_program(ast);
  // vm_dump_inst(prog.items, prog.used);

  while (!vm.halt) {
    // vm_dump_stack_n_inst(vm, prog.items);
    VM_Instruction_Signal sig = vm_exec_prog(&vm, prog.items, prog.used);
    if (sig != INST_SIG_OK) {
      printf("%s\n", vm_sig_as_cstr(sig));
      // vm_dump_stack_n_inst(vm, prog.items);
      exit(1);
    }
  }

  DB_Page *thispage = vm.stack[0].as_ptr;        // extract page from stack;
  thispage->pageheader.pageno = global_pageno++; // set its pageno
  int idx = hashmap_append(&mem.pagedir, thispage->pageheader.objname);
  KV_append(mem.pagedir.items[idx], thispage->pageheader.pageno);

  SQL_Col_Def_Array *col_def_arr = vm.stack[1].as_ptr;
  mem_append(&mem, *thispage);
  idx = hashmap_append(&table_definition, thispage->pageheader.objname);
  KV_append(table_definition.items[idx], (uint64_t)col_def_arr);

  free(prog.items);

  // TEST: TEST INSERT
  token = tokenize("insert into t1 values(1337,'hello "
                   "world', 'haha you suck')");
  ast = parse(token);
  // print_ast_tree(ast, 1);
  prog = sql_program(ast);
  // vm_dump_inst(prog.items, prog.used);

  vm.stack_size = 0;
  vm.halt = 0;
  vm.pinst = 0;
  while (!vm.halt) {
    // vm_dump_stack_n_inst(vm, prog.items);
    VM_Instruction_Signal sig = vm_exec_prog(&vm, prog.items, prog.used);
    if (sig != INST_SIG_OK) {
      printf("%s\n", vm_sig_as_cstr(sig));
      // vm_dump_stack_n_inst(vm, prog.items);
      exit(1);
    }
  }
  char *ins_tabname = vm.stack[0].as_ptr;
  SQL_Insert_Val_Array *ins_val_arr = vm.stack[1].as_ptr;
  // insert values checked against table definition;
  assert(col_def_arr->used == ins_val_arr->used &&
         "Insert values number mismatch with table definition");
  for (int i = 0; i < ins_val_arr->used; i++) {
    assert(col_def_arr->items[i].type == ins_val_arr->items[i].datatype &&
           "Insert values type mismatch with table definition");
  }
  idx = hashmap_find(&mem.pagedir, ins_tabname);
  int pageno = mem.pagedir.items[idx].pageno[0];
  char pageno_str[32];
  sprintf(pageno_str, "%d", pageno);
  idx = hashmap_find(&mem.pagetable, pageno_str);
  int memloc = mem.pagetable.items[idx].pageno[0];

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
  sql_ins_buf_to_page(mem.buffer + memloc, buf, tupesize);

  // TEST: TEST SELECT;
  token = tokenize("select * from t1 where id = 1337 and txt = 'hello world' ");
  print_token(token);
  ast = parse(token);
  print_ast_tree(ast, 1);
  prog = sql_program(ast);

  // NOTE: FREE HEAP MEMORY

  region_free(&mem_ast);

  free(ins_val_arr->items);
  free(col_def_arr->items);
  free(ins_val_arr);
  free(col_def_arr);
  hashmap_free(&mem.pagetable);
  hashmap_free(&mem.pagedir);
  free(mem.buffer);
  hashmap_free(&table_definition);
  free(prog.items);
  free(thispage);
  return 0;
}
