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
  INST_IADD, // I for integer, F for double
  INST_ISUB,
  INST_IDIV,
  INST_IMUL,
  INST_FADD, // I for integer, F for double
  INST_FSUB,
  INST_FDIV,
  INST_FMUL,
  INST_JMP,
  INST_JNZ,
  INST_EQ,
  INST_HLT,
  INST_CREATE_TABLE,  // use 2 stack value at the top, and consume them,
                      // it's tablename and tuplesoze, then the address of
                      // DB_Page malloced is push to the stack
  INST_DEFINE_COLUMN, // use 3 stack values at the top, consume top 3, they are
                      // colname, length, type
  INST_COUNT,
} VM_Instruction_Type;

typedef union {
  int64_t as_i64;
  uint64_t as_u64;
  double as_f64;
  void *as_ptr;
} Word;

static_assert(sizeof(Word) == 8, "Word size should be 64 bits");

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
void vm_dump_stack_n_inst(VM vm, VM_Instruction program[]);
const char *vm_sig_as_cstr(VM_Instruction_Signal signal);
const char *vm_ins_as_cstr(VM_Instruction ins);
VM_Instruction vm_push(Word operand);
VM_Instruction vm_dup(size_t pstack);
VM_Instruction vm_iadd();
VM_Instruction vm_isub();
VM_Instruction vm_imul();
VM_Instruction vm_idiv();
VM_Instruction vm_fadd();
VM_Instruction vm_fsub();
VM_Instruction vm_fmul();
VM_Instruction vm_fdiv();
VM_Instruction vm_jmp(size_t pinst);
VM_Instruction vm_hlt();
VM_Instruction vm_eq();
VM_Instruction vm_jnz(size_t pinst);
VM_Instruction vm_nop();
VM_Instruction vm_swap(size_t pstack);
VM_Instruction vm_create_table();
VM_Instruction vm_define_column();
