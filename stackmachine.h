#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define VM_STACK_SIZE 16
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
  INST_DUP, // dup a value at the offset of current stack position
  INST_ADD,
  INST_SUB,
  INST_DIV,
  INST_MUL,
  INST_JMP,
  INST_JNZ,
  INST_EQ,
  INST_HALT,
  INST_COUNT,
} VM_Instruction_Type;

typedef struct {
  VM_Instruction_Type inst_type;
  int64_t operand; // to accept any value
} VM_Instruction;

typedef struct {
  int64_t stack[VM_STACK_SIZE]; // stack that basiclly accept any value
  size_t stack_size;
  int64_t pinst; // points to a position in the program
  int8_t halt;
} VM;

VM_Instruction_Signal vm_exec_prog(VM *vm, VM_Instruction program[],
                                   size_t program_size);
void vm_dump_stack(VM vm);
void vm_dump_stack_n_inst(VM vm, VM_Instruction program[]);
const char *vm_sig_as_cstr(VM_Instruction_Signal signal);
const char *vm_ins_as_cstr(VM_Instruction ins);
VM_Instruction vm_push(int64_t operand);
VM_Instruction vm_dup(size_t pstack);
VM_Instruction vm_add();
VM_Instruction vm_sub();
VM_Instruction vm_mul();
VM_Instruction vm_div();
VM_Instruction vm_jmp(size_t pinst);
VM_Instruction vm_halt();
VM_Instruction vm_eq();
VM_Instruction vm_jnz(size_t pinst);
VM_Instruction vm_nop();
