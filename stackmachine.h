#pragma once
#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define VM_STACK_SIZE 1024
#define ARRAY_SIZE(x) sizeof(x) / sizeof((x)[0])

typedef enum {
  INST_SIG_OK,
  INST_SIG_OVERFLOW, // caused by stack or float
  INST_SIG_UNDERFOW, // cause by stack or float
  INST_SIG_UNKNOWN,
  INST_SIG_ILLEGAL, // when pinst lt 0 or pinst gt program size
  INST_SIG_FPE,
  INST_SIG_COUNT,
} VM_Instruction_Signal;

typedef enum {
  INST_NOP = 0, // need this so that, zero initialized array can be used as a
                // program full of no operation
  INST_PUSH,
  INST_DUP,  // dup a value at the offset of top stack position
  INST_SWAP, // swap top of stack and what operand is pointing to, similar to
             // dup
  INST_ADD,  // I for integer, F for double
  INST_SUB,
  INST_DIV,
  INST_MUL,
  INST_JMP,
  INST_JNZ,
  // in case sql passes char type, push datatupe onto stack for extra info
  INST_EQ,
  INST_NEQ,
  INST_LEQ,
  INST_GEQ,
  INST_LT,
  INST_GT,
  INST_OR,
  INST_AND,
  INST_NOT,
  INST_CREATE_TABLE, // use 1 stack value at the top, and consume it,
                     // it's tablename, then the address of
                     // DB_Page malloced is push to the stack , also the address
                     // of a struct representing table defnition is pushed to
                     // stack
  INST_DEFINE_COLUMN, // use 4 stack values at the top, consume top 3, they are
                      // table defnition struct,colname, type, length
  INST_INSERT_PREP,   // malloc struct for following operation
  INST_INSERT_VALUE,  // use 3 stack values, consume 2;
  // they are struct, datatype and value;
  INST_FROM_TABLES, // put Node * and MemArea * on stack;
  // take all the values on the stack and put the table_list
  // struct and the address of a row generator on top of the
  // stack;
  INST_GENERATOR, // call sql_cartesian_rowset_generator
  INST_FETCH_VAL, // fetch the value of tabname.colname
  INST_PUSH_RES_NZ,
  INST_PUSH_SELECT_LIST, // push to SQL_Select_LIST
  INST_SELECT_PROJECT,   // project to filtered rowset to result set
  INST_HLT_IFZ,
  INST_HLT,
  INST_COUNT,
} VM_Instruction_Type;

typedef enum {
  WORD_NIL,
  WORD_I64,
  WORD_U64,
  WORD_F64,
  WORD_PTR,
  WORD_STR,
  WORD_CNT,
} Word_Tag;

typedef struct {
  Word_Tag tag;
  union {
    int64_t as_i64;
    int64_t as_u64;
    double as_f64;
    void *as_ptr;
  } val;
} Word;

typedef struct {
  VM_Instruction_Type inst_type;
  Word operand; // to accept any value
} VM_Instruction;

typedef struct {
  Word stack[VM_STACK_SIZE]; // stack that basiclly accept any value
  size_t stack_size;
  uint64_t pinst; // points to a position in the program
  int8_t halt;
} VM;

VM_Instruction_Signal vm_exec_prog(VM *vm, VM_Instruction program[],
                                   size_t program_size);
void vm_dump_stack(VM vm);
void vm_dump_inst(VM_Instruction program[], int program_size);
void vm_dump_stack_n_inst(VM vm, VM_Instruction program[]);
void vm_dump_word(Word word);
const char *vm_sig_as_cstr(VM_Instruction_Signal signal);
const char *vm_ins_as_cstr(VM_Instruction ins);
const char *vm_word_type_as_cstr(Word_Tag tag);
VM_Instruction vm_push(Word operand);
VM_Instruction vm_dup(size_t pstack);
VM_Instruction vm_add();
VM_Instruction vm_sub();
VM_Instruction vm_mul();
VM_Instruction vm_div();
VM_Instruction vm_jmp(size_t pinst);
VM_Instruction vm_hlt();
VM_Instruction vm_eq();
VM_Instruction vm_neq();
VM_Instruction vm_lt();
VM_Instruction vm_leq();
VM_Instruction vm_gt();
VM_Instruction vm_geq();
VM_Instruction vm_jnz(size_t pinst);
VM_Instruction vm_nop();
VM_Instruction vm_swap(size_t pstack);
VM_Instruction vm_sql_from_table();
VM_Instruction vm_not();
VM_Instruction vm_and();
VM_Instruction vm_or();
VM_Instruction vm_gen();
VM_Instruction vm_fetchval();
VM_Instruction vm_hlt_ifz();
VM_Instruction vm_push_res_nz();
VM_Instruction vm_push_sel_list();
VM_Instruction vm_select_project();
