#include "sqldb.h"
#include <assert.h>
#include <float.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VM_Instruction_Signal vm_exec_prog(VM *vm, VM_Instruction program[],
                                   size_t program_size) {

  if (vm->pinst < 0 || vm->pinst >= (int64_t)program_size) {
    return INST_SIG_ILLEGAL;
  }
  VM_Instruction instruction = program[vm->pinst];
  switch (instruction.inst_type) {
  case INST_NOP:
    vm->pinst++;
    break;
  case INST_PUSH:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    vm->stack[vm->stack_size++] = instruction.operand;
    vm->pinst++;
    break;

  case INST_ADD:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2] += vm->stack[vm->stack_size - 1];
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_SUB:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2] -= vm->stack[vm->stack_size - 1];
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_MUL:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2] *= vm->stack[vm->stack_size - 1];
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_DIV:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1] == 0) {
      return INST_SIG_FPE;
    }

    vm->stack[vm->stack_size - 2] /= vm->stack[vm->stack_size - 1];
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_JMP:
    if (instruction.operand < 0) {
      return INST_SIG_ILLEGAL;
    }
    vm->pinst = instruction.operand;
    break;

  case INST_HALT:
    vm->halt = 1;
    break;

  case INST_EQ:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    vm->stack[vm->stack_size - 2] =
        vm->stack[vm->stack_size - 1] == vm->stack[vm->stack_size - 2];
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_JNZ:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1]) {
      vm->stack_size--;
      vm->pinst = instruction.operand;
    } else {
      vm->pinst++;
    }
    break;

  case INST_DUP:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    if (vm->stack_size - instruction.operand <= 0) {
      return INST_SIG_UNDERFOW;
    }
    if (instruction.operand < 0) {
      return INST_SIG_ILLEGAL;
    }
    vm->stack[vm->stack_size] =
        vm->stack[vm->stack_size - instruction.operand - 1];
    vm->stack_size++;
    vm->pinst++;

    break;

    // default:
    //   return INST_SIG_UNKNOWN;
  }

  return INST_SIG_OK;
}

void vm_dump_stack(VM vm) {
  puts("Stack:");
  for (size_t i = 0; i < vm.stack_size; i++) {
    printf("[%3lu]:"
           "  "
           "%ld\n",
           i, vm.stack[i]);
  }
}

void vm_dump_stack_n_inst(VM vm, VM_Instruction program[]) {
  puts("Stack:");
  for (size_t i = 0; i < vm.stack_size; i++) {
    printf("[%3lu]:"
           "  "
           "%ld\n",
           i, vm.stack[i]);
  }
  puts("Instruction:");
  printf(
      "[%3lu]:"
      "  "
      "%s\n",
      vm.pinst,
      vm_ins_as_cstr(program[vm.pinst])); // TODO: if %s is null, then UB, but
                                          // gcc works because printf can handle
                                          // this, but not optimized puts
}

const char *vm_sig_as_cstr(VM_Instruction_Signal signal) {
  switch (signal) {
  case INST_SIG_OK:
    return "INST_SIG_OK";
    break;
  case INST_SIG_OVERFLOW:
    return "INST_SIG_OVERFLOW";
    break;
  case INST_SIG_UNDERFOW:
    return "INST_SIG_UNDERFOW";
    break;
  case INST_SIG_UNKNOWN:
    return "INST_SIG_UNKNOWN";
    break;
  case INST_SIG_ILLEGAL:
    return "INST_SIG_ILLEGAL";
    break;
  case INST_SIG_FPE:
    return "INST_SIG_FPE";
    break;
  }
  return NULL;
}

const char *vm_ins_as_cstr(VM_Instruction ins) {
  switch (ins.inst_type) {
  case INST_NOP:
    return "INST_NOP";
  case INST_PUSH:
    return "INST_PUSH";
  case INST_ADD:
    return "INST_ADD";
  case INST_SUB:
    return "INST_SUB";
  case INST_DIV:
    return "INST_DIV";
  case INST_MUL:
    return "INST_MUL";
  case INST_JMP:
    return "INST_JMP";
  case INST_HALT:
    return "INST_HALT";
  case INST_JNZ:
    return "INST_JNZ";
  case INST_EQ:
    return "INST_EQ";
  case INST_DUP:
    return "INST_DUP";
  }
  return NULL;
}

VM_Instruction vm_push(int64_t operand) {
  return (VM_Instruction){
      .inst_type = INST_PUSH,
      .operand = operand,
  };
}

VM_Instruction vm_add() {
  return (VM_Instruction){
      .inst_type = INST_ADD,
  };
}

VM_Instruction vm_jmp(size_t pinst) {
  return (VM_Instruction){.inst_type = INST_JMP, .operand = pinst};
}
VM_Instruction vm_halt() { return (VM_Instruction){.inst_type = INST_HALT}; }

VM_Instruction vm_sub() {
  return (VM_Instruction){
      .inst_type = INST_SUB,
  };
}
VM_Instruction vm_mul() {
  return (VM_Instruction){
      .inst_type = INST_MUL,
  };
}
VM_Instruction vm_div() {
  return (VM_Instruction){
      .inst_type = INST_DIV,
  };
}

VM_Instruction vm_eq() {
  return (VM_Instruction){
      .inst_type = INST_EQ,
  };
}

VM_Instruction vm_jnz(size_t pinst) {
  return (VM_Instruction){.inst_type = INST_JNZ, .operand = pinst};
}
VM_Instruction vm_dup(size_t pstack) {
  return (VM_Instruction){.inst_type = INST_DUP, .operand = pstack};
}
VM_Instruction vm_nop() { return (VM_Instruction){0}; }
