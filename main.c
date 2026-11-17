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
#include <strings.h>
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
      vm_push((Word){.tag = WORD_I64, .val.as_i64 = 0}),
      vm_not(),
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
    // fgetc(stdin);
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
  //
  // sql_exec_create(
  //     "create table t1 (t1_id integer, t1_txt char(15), t1_note char(16))",
  //     &vm, &mem, &table_definition);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;
  sql_exec_sql(
      "create table t1 (t1_id integer, t1_txt char(15), t1_note char(16))", &vm,
      &mem, &table_definition);

  // sql_exec_create(
  //     "create table t2 (t2_id integer, t2_txt char(15), t2remark char(16))",
  //     &vm, &mem, &table_definition);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;

  sql_exec_sql(
      "create table t2 (t2_id integer, t2_txt char(15), t2remark char(16))",
      &vm, &mem, &table_definition);

  // sql_exec_create("create table t3 (t3_tablename char(32), t3_id integer)",
  // &vm,
  //                 &mem, &table_definition);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;
  sql_exec_sql("create table t3 (t3_tablename char(32), t3_id integer)", &vm,
               &mem, &table_definition);

  // TEST: TEST INSERT
  //
  // sql_exec_insert("insert into t1 values(0,'this is t1:0', 'you forget')",
  // &vm,
  //                 &table_definition, &mem);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;
  // sql_exec_insert("insert into t1 values(1,'this is t1:1', 'thousand
  // things')",
  //                 &vm, &table_definition, &mem);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;
  // sql_exec_insert("insert into t1 values(2,'this is t1:2', 'everyday.')",
  // &vm,
  //                 &table_definition, &mem);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;
  // sql_exec_insert("insert into  t3 values('t3:0, make sure', 0)", &vm,
  //                 &table_definition, &mem);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;
  // sql_exec_insert("insert into t3  values('t3:1, this is one of them', 1)",
  // &vm,
  //                 &table_definition, &mem);
  // vm.stack_size = 0, vm.halt = 0, vm.pinst = 0;

  sql_exec_sql("insert into t1 values(0,'this is t1:0', 'you forget')", &vm,
               &mem, &table_definition);
  sql_exec_sql("insert into t1 values(1,'this is t1:1', 'thousand things')",
               &vm, &mem, &table_definition);
  sql_exec_sql("insert into t1 values(2,'this is t1:2', 'everyday.')", &vm,
               &mem, &table_definition);
  sql_exec_sql("insert into  t3 values('t3:0, make sure', 0)", &vm, &mem,
               &table_definition);
  sql_exec_sql("insert into t3  values('t3:1, this is one of them', 1)", &vm,
               &mem, &table_definition);

  // TEST: TEST SELECT;
  //
  // sql_exec_select("select t2.*,t3.*,432*0.52,'ohnepixel',t3.t3_tablename from
  // "
  //                 "t2,t3 where 1=1",
  //                 &table_definition, &vm, &mem);
  // sql_exec_sql("from t1,t3 select t1.* ", &vm, &mem, &table_definition);
  //
  //
  //

  sql_exec_sql("create table pokemon (id integer, name char(16) )", &vm, &mem,
               &table_definition);
  for (int i = 0; i < 5; i++) {
    char pokemon_sql[129] = {0};
    snprintf(pokemon_sql, 128, "insert into pokemon values (%d, '%s')", i,
             pokemon[i]);
    sql_exec_sql(pokemon_sql, &vm, &mem, &table_definition);
  }

  sql_exec_sql("select (-1) * 3 from pokemon", &vm, &mem, &table_definition);
  // NOTE: SQL parser
  //
  region_free(&mem_ast);
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
    if (strcasecmp(command, "exit") == 0 || strcasecmp(command, "quit") == 0) {
      break;
    }
    sql_exec_sql(command, &vm, &mem, &table_definition);
    free(command);
    region_free(&mem_ast);
  }
  free(command);
  //
  //
  // NOTE: FREE HEAP MEMORY

  region_free(&mem_ast);
  hashmap_free(&mem.pagetable);
  hashmap_free(&mem.pagedir);
  free(mem.buffer);
  SQL_Col_Def_Array *to_free =
      (SQL_Col_Def_Array *)table_definition.items[0].pageno;
  for (int i = 0; i < table_definition.capa; i++) {
    if (table_definition.items[i].cnt != 0) {
      SQL_Col_Def_Array *to_free =
          (SQL_Col_Def_Array *)table_definition.items[i].pageno[0];
      free(to_free->items);
      free(to_free);
    }
  }
  free(to_free);
  hashmap_free(&table_definition);
  return 0;
}
