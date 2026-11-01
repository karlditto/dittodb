#include "stackmachine.h"
#include "hashmap.h"
#include "parse.h"
#include "sqldb.h"
#include "sqlengine.h"
#include <assert.h>
#include <float.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VM_Instruction_Signal vm_exec_prog(VM *vm, VM_Instruction program[],
                                   size_t program_size) {

  if (vm->pinst >= (int64_t)program_size) {
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

  case INST_IADD:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2].as_i64 +=
        vm->stack[vm->stack_size - 1].as_i64;
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_ISUB:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2].as_i64 -=
        vm->stack[vm->stack_size - 1].as_i64;
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_IMUL:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2].as_i64 *=
        vm->stack[vm->stack_size - 1].as_i64;
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_IDIV:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].as_i64 == 0) {
      return INST_SIG_FPE;
    }

    vm->stack[vm->stack_size - 2].as_i64 /=
        vm->stack[vm->stack_size - 1].as_i64;
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_FADD:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2].as_f64 +=
        vm->stack[vm->stack_size - 1].as_f64;
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_FSUB:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2].as_f64 -=
        vm->stack[vm->stack_size - 1].as_f64;
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_FMUL:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2].as_f64 *=
        vm->stack[vm->stack_size - 1].as_f64;
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_FDIV:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    vm->stack[vm->stack_size - 2].as_f64 /=
        vm->stack[vm->stack_size - 1].as_f64;
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_JMP:
    vm->pinst = instruction.operand.as_u64;
    break;

  case INST_HLT:
    vm->halt = 1;
    break;

  case INST_EQ:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    vm->stack[vm->stack_size - 2].as_u64 =
        vm->stack[vm->stack_size - 1].as_u64 ==
        vm->stack[vm->stack_size - 2].as_u64;
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_JNZ:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].as_u64) {
      vm->stack_size--;
      vm->pinst = instruction.operand.as_u64;
    } else {
      vm->pinst++;
    }
    break;

  case INST_DUP:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    if (vm->stack_size - instruction.operand.as_i64 <= 0) {
      return INST_SIG_UNDERFOW;
    }
    if (instruction.operand.as_i64 < 0) {
      return INST_SIG_ILLEGAL;
    }
    vm->stack[vm->stack_size] =
        vm->stack[vm->stack_size - instruction.operand.as_i64 - 1];
    vm->stack_size++;
    vm->pinst++;

    break;

  case INST_SWAP:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    if (vm->stack_size - instruction.operand.as_i64 <= 0) {
      return INST_SIG_UNDERFOW;
    }
    if (instruction.operand.as_i64 < 0) {
      return INST_SIG_ILLEGAL;
    }
    Word tmp = vm->stack[vm->stack_size - 1];
    vm->stack[vm->stack_size - 1] =
        vm->stack[vm->stack_size - instruction.operand.as_i64 - 1];
    vm->stack[vm->stack_size - instruction.operand.as_i64 - 1] = tmp;
    vm->pinst++;

    break;

  case INST_CREATE_TABLE:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    DB_Page *page = calloc(1, sizeof(DB_Page));
    init_page(page, vm->stack[vm->stack_size - 1].as_ptr);
    vm->stack[vm->stack_size - 1].as_ptr = page;
    SQL_Col_Def_Array *col_def_arr = calloc(1, sizeof(SQL_Col_Def_Array));
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    vm->stack[vm->stack_size++].as_ptr = col_def_arr;
    vm->pinst++;
    break;

  case INST_DEFINE_COLUMN:
    if (vm->stack_size < 4) {
      return INST_SIG_UNDERFOW;
    }
    SQL_Col_Def_Array *col_def_arr_tmp = vm->stack[vm->stack_size - 4].as_ptr;
    SQL_Col_Def col_def_tmp = {0};
    strncpy(col_def_tmp.colname, vm->stack[vm->stack_size - 3].as_ptr,
            OBJ_NAME_LEN);
    col_def_tmp.type = vm->stack[vm->stack_size - 2].as_i64;
    col_def_tmp.len = vm->stack[vm->stack_size - 1].as_i64;
    sql_push((*col_def_arr_tmp), col_def_tmp);
    vm->stack_size -= 3;
    vm->pinst++;
    break;

  case INST_INSERT_PREP:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    SQL_Insert_Val_Array *ins_val_arr = calloc(1, sizeof(SQL_Insert_Val_Array));
    vm->stack[vm->stack_size++].as_ptr = ins_val_arr;
    vm->pinst++;

    break;

  case INST_INSERT_VALUE:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    SQL_Insert_Val_Array *ins_val_arr_tmp =
        vm->stack[vm->stack_size - 3].as_ptr;
    SQL_Insert_Val ins_val = {0};
    if (vm->stack[vm->stack_size - 2].as_i64 == INTEGER) {
      ins_val.datatype = INTEGER;
      ins_val.val.as_int = vm->stack[vm->stack_size - 1].as_i64;
    }
    if (vm->stack[vm->stack_size - 2].as_i64 == FLOAT) {
      ins_val.datatype = FLOAT;
      ins_val.val.as_float = vm->stack[vm->stack_size - 1].as_f64;
    }
    if (vm->stack[vm->stack_size - 2].as_i64 == STRING) {
      ins_val.datatype = STRING;
      ins_val.val.as_str = vm->stack[vm->stack_size - 1].as_ptr;
    }
    sql_push((*ins_val_arr_tmp), ins_val);
    vm->stack_size -= 2;
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
           "i64:%ld u64:%lu f64:%lf ptr:%p\n",
           i, vm.stack[i].as_i64, vm.stack[i].as_u64, vm.stack[i].as_f64,
           vm.stack[i].as_ptr);
  }
}

void vm_dump_inst(VM_Instruction *program, int program_size) {
  puts("Program instruction");
  for (int i = 0; i < program_size; i++) {
    printf("[%3d]:"
           "%s\n",
           i, vm_ins_as_cstr(program[i]));
  }
}

void vm_dump_stack_n_inst(VM vm, VM_Instruction program[]) {
  puts("Stack:");
  for (size_t i = 0; i < vm.stack_size; i++) {
    printf("[%3lu]:"
           "  "
           "i64:%ld u64:%lu f64:%lf ptr:%p\n",
           i, vm.stack[i].as_i64, vm.stack[i].as_u64, vm.stack[i].as_f64,
           vm.stack[i].as_ptr);
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
  case INST_CREATE_TABLE:
    return "INST_CREATE_TABLE";
  case INST_DEFINE_COLUMN:
    return "INST_DEFINE_COLUMN";
  case INST_PUSH:
    return "INST_PUSH";
  case INST_IADD:
    return "INST_IADD";
  case INST_ISUB:
    return "INST_ISUB";
  case INST_IDIV:
    return "INST_IDIV";
  case INST_IMUL:
    return "INST_IMUL";
  case INST_FADD:
    return "INST_FADD";
  case INST_FSUB:
    return "INST_FSUB";
  case INST_FDIV:
    return "INST_FDIV";
  case INST_FMUL:
    return "INST_FMUL";
  case INST_JMP:
    return "INST_JMP";
  case INST_HLT:
    return "INST_HLT";
  case INST_JNZ:
    return "INST_JNZ";
  case INST_EQ:
    return "INST_EQ";
  case INST_DUP:
    return "INST_DUP";
  case INST_SWAP:
    return "INST_SWAP";
  case INST_INSERT_VALUE:
    return "INST_INSERT_VALUE";
  case INST_INSERT_PREP:
    return "INST_INSERT_PREP";
  }
  return NULL;
}

VM_Instruction vm_push(Word operand) {
  return (VM_Instruction){
      .inst_type = INST_PUSH,
      .operand = operand,
  };
}

VM_Instruction vm_iadd() {
  return (VM_Instruction){
      .inst_type = INST_IADD,
  };
}
VM_Instruction vm_fadd() {
  return (VM_Instruction){
      .inst_type = INST_FADD,
  };
}
VM_Instruction vm_jmp(size_t pinst) {
  return (VM_Instruction){.inst_type = INST_JMP, .operand = {.as_i64 = pinst}};
}
VM_Instruction vm_hlt() { return (VM_Instruction){.inst_type = INST_HLT}; }

VM_Instruction vm_isub() {
  return (VM_Instruction){
      .inst_type = INST_ISUB,
  };
}
VM_Instruction vm_imul() {
  return (VM_Instruction){
      .inst_type = INST_IMUL,
  };
}
VM_Instruction vm_idiv() {
  return (VM_Instruction){
      .inst_type = INST_IDIV,
  };
}

VM_Instruction vm_fsub() {
  return (VM_Instruction){
      .inst_type = INST_FSUB,
  };
}
VM_Instruction vm_fmul() {
  return (VM_Instruction){
      .inst_type = INST_FMUL,
  };
}
VM_Instruction vm_fdiv() {
  return (VM_Instruction){
      .inst_type = INST_FDIV,
  };
}

VM_Instruction vm_eq() {
  return (VM_Instruction){
      .inst_type = INST_EQ,
  };
}

VM_Instruction vm_jnz(size_t pinst) {
  return (VM_Instruction){.inst_type = INST_JNZ, .operand = {.as_i64 = pinst}};
}
VM_Instruction vm_dup(size_t pstack) {
  return (VM_Instruction){.inst_type = INST_DUP, .operand = {.as_i64 = pstack}};
}
VM_Instruction vm_nop() { return (VM_Instruction){0}; }
VM_Instruction vm_swap(size_t pstack) {
  return (VM_Instruction){.inst_type = INST_SWAP,
                          .operand = {.as_i64 = pstack}};
}
VM_Instruction vm_create_table() {
  return (VM_Instruction){.inst_type = INST_CREATE_TABLE};
}
VM_Instruction vm_define_column() {
  return (VM_Instruction){.inst_type = INST_DEFINE_COLUMN};
}
