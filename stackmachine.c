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
#include <strings.h>
#include <time.h>

VM_Instruction_Signal vm_exec_prog(VM *vm, VM_Instruction program[],
                                   size_t program_size) {

  if (vm->pinst >= (uint64_t)program_size) {
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
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 +=
          vm->stack[vm->stack_size - 1].val.as_f64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_i64 +=
          vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 +=
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_F64,
          .val.as_f64 = vm->stack[vm->stack_size - 1].val.as_f64 +
                        (double)vm->stack[vm->stack_size - 2].val.as_i64};
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_SUB:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 -=
          vm->stack[vm->stack_size - 1].val.as_f64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_i64 -=
          vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 -=
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_F64,
          .val.as_f64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 -
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else {
      return INST_SIG_ILLEGAL;
    }

    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_MUL:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }

    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 *=
          vm->stack[vm->stack_size - 1].val.as_f64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_i64 *=
          vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 *=
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_F64,
          .val.as_f64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 *
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;
  case INST_DIV:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
        vm->stack[vm->stack_size - 1].val.as_i64 == 0) {
      return INST_SIG_FPE;
    }

    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 /=
          vm->stack[vm->stack_size - 1].val.as_f64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_i64 /=
          vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_f64 /=
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_F64,
          .val.as_f64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 /
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_JMP:
    vm->pinst = instruction.operand.val.as_i64;
    break;

  case INST_HLT:
    vm->halt = 1;
    break;
  case INST_HLT_IFZ:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }

    if (vm->stack[vm->stack_size - 1].val.as_u64 == 0) {
      vm->halt = 1;
      vm->pinst++;
    } else {
      vm->pinst++;
    }
    break;

  case INST_AND:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    vm->stack[vm->stack_size - 2].val.as_u64 =
        vm->stack[vm->stack_size - 1].val.as_u64 &&
        vm->stack[vm->stack_size - 2].val.as_u64;
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_OR:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    vm->stack[vm->stack_size - 2].val.as_u64 =
        vm->stack[vm->stack_size - 1].val.as_u64 ||
        vm->stack[vm->stack_size - 2].val.as_u64;
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_NOT:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    vm->stack[vm->stack_size - 1].val.as_u64 =
        !vm->stack[vm->stack_size - 1].val.as_u64;
    vm->pinst++;
    break;

  case INST_EQ:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 ==
          vm->stack[vm->stack_size - 1].val.as_f64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_i64 ==
          vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 ==
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_U64,
          .val.as_u64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 ==
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_STR &&
               vm->stack[vm->stack_size - 2].tag == WORD_STR) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_I64,
          .val.as_i64 = strcmp(vm->stack[vm->stack_size - 2].val.as_ptr,
                               vm->stack[vm->stack_size - 1].val.as_ptr) == 0,
      };
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_NEQ:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 !=
          vm->stack[vm->stack_size - 1].val.as_f64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_i64 !=
          vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 !=
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_U64,
          .val.as_u64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 !=
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_STR &&
               vm->stack[vm->stack_size - 2].tag == WORD_STR) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_I64,
          .val.as_i64 = !strcmp(vm->stack[vm->stack_size - 2].val.as_ptr,
                                vm->stack[vm->stack_size - 1].val.as_ptr),
      };
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_LEQ:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 <=
          vm->stack[vm->stack_size - 1].val.as_f64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_i64 <=
          vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 <=
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_U64,
          .val.as_u64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 <=
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_STR &&
               vm->stack[vm->stack_size - 2].tag == WORD_STR) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_I64,
          .val.as_i64 = strcmp(vm->stack[vm->stack_size - 2].val.as_ptr,
                               vm->stack[vm->stack_size - 1].val.as_ptr) <= 0,
      };
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_GEQ:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 >=
          vm->stack[vm->stack_size - 1].val.as_f64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_i64 >=
          vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 >=
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_U64,
          .val.as_u64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 >=
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_STR &&
               vm->stack[vm->stack_size - 2].tag == WORD_STR) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_I64,
          .val.as_i64 = strcmp(vm->stack[vm->stack_size - 2].val.as_ptr,
                               vm->stack[vm->stack_size - 1].val.as_ptr) >= 0,
      };
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_GT:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 >
          vm->stack[vm->stack_size - 1].val.as_f64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_i64 >
          vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 >
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_U64,
          .val.as_u64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 >
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_STR &&
               vm->stack[vm->stack_size - 2].tag == WORD_STR) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_I64,
          .val.as_i64 = strcmp(vm->stack[vm->stack_size - 2].val.as_ptr,
                               vm->stack[vm->stack_size - 1].val.as_ptr) > 0,
      };
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_LT:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
        vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 <
          vm->stack[vm->stack_size - 1].val.as_f64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_i64 <
          vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_I64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_F64) {
      vm->stack[vm->stack_size - 2].val.as_u64 =
          vm->stack[vm->stack_size - 2].val.as_f64 <
          (double)vm->stack[vm->stack_size - 1].val.as_i64;
      vm->stack[vm->stack_size - 2].tag = WORD_U64;
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_F64 &&
               vm->stack[vm->stack_size - 2].tag == WORD_I64) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_U64,
          .val.as_u64 = (double)vm->stack[vm->stack_size - 2].val.as_i64 <
                        vm->stack[vm->stack_size - 1].val.as_f64};
    } else if (vm->stack[vm->stack_size - 1].tag == WORD_STR &&
               vm->stack[vm->stack_size - 2].tag == WORD_STR) {
      vm->stack[vm->stack_size - 2] = (Word){
          .tag = WORD_I64,
          .val.as_i64 = strcmp(vm->stack[vm->stack_size - 2].val.as_ptr,
                               vm->stack[vm->stack_size - 1].val.as_ptr) < 0,
      };
    } else {
      return INST_SIG_ILLEGAL;
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_JNZ:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].val.as_u64) {
      vm->stack_size--;
      vm->pinst = instruction.operand.val.as_u64;
    } else {
      vm->pinst++;
    }
    break;

  case INST_DUP:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    if (vm->stack_size - instruction.operand.val.as_i64 <= 0) {
      return INST_SIG_UNDERFOW;
    }
    if (instruction.operand.val.as_i64 < 0) {
      return INST_SIG_ILLEGAL;
    }
    vm->stack[vm->stack_size] =
        vm->stack[vm->stack_size - instruction.operand.val.as_i64 - 1];
    vm->stack_size++;
    vm->pinst++;

    break;

  case INST_SWAP:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    if (vm->stack_size - instruction.operand.val.as_i64 <= 0) {
      return INST_SIG_UNDERFOW;
    }
    if (instruction.operand.val.as_i64 < 0) {
      return INST_SIG_ILLEGAL;
    }
    Word tmp = vm->stack[vm->stack_size - 1];
    vm->stack[vm->stack_size - 1] =
        vm->stack[vm->stack_size - instruction.operand.val.as_i64 - 1];
    vm->stack[vm->stack_size - instruction.operand.val.as_i64 - 1] = tmp;
    vm->pinst++;

    break;

  case INST_CREATE_TABLE:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    DB_Page *page = calloc(1, sizeof(DB_Page));
    init_page(page, vm->stack[vm->stack_size - 1].val.as_ptr);
    vm->stack[vm->stack_size - 1].val.as_ptr = page;
    SQL_Col_Def_Array *col_def_arr = calloc(1, sizeof(SQL_Col_Def_Array));
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    vm->stack[vm->stack_size++].val.as_ptr = col_def_arr;
    vm->pinst++;
    break;

  case INST_DEFINE_COLUMN:
    if (vm->stack_size < 4) {
      return INST_SIG_UNDERFOW;
    }
    SQL_Col_Def_Array *col_def_arr_tmp =
        vm->stack[vm->stack_size - 4].val.as_ptr;
    SQL_Col_Def col_def_tmp = {0};
    strncpy(col_def_tmp.colname, vm->stack[vm->stack_size - 3].val.as_ptr,
            OBJ_NAME_LEN);
    col_def_tmp.type = vm->stack[vm->stack_size - 2].val.as_i64;
    col_def_tmp.len = vm->stack[vm->stack_size - 1].val.as_i64;
    sql_push((*col_def_arr_tmp), col_def_tmp);
    vm->stack_size -= 3;
    vm->pinst++;
    break;

  case INST_INSERT_PREP:
    if (vm->stack_size >= VM_STACK_SIZE) {
      return INST_SIG_OVERFLOW;
    }
    SQL_Insert_Val_Array *ins_val_arr = calloc(1, sizeof(SQL_Insert_Val_Array));
    vm->stack[vm->stack_size++].val.as_ptr = ins_val_arr;
    vm->pinst++;

    break;

  case INST_INSERT_VALUE:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    SQL_Insert_Val_Array *ins_val_arr_tmp =
        vm->stack[vm->stack_size - 3].val.as_ptr;
    SQL_Insert_Val ins_val = {0};
    if (vm->stack[vm->stack_size - 2].val.as_i64 == INTEGER) {
      ins_val.datatype = INTEGER;
      ins_val.val.as_int = vm->stack[vm->stack_size - 1].val.as_i64;
    }
    if (vm->stack[vm->stack_size - 2].val.as_i64 == FLOAT) {
      ins_val.datatype = FLOAT;
      ins_val.val.as_float = vm->stack[vm->stack_size - 1].val.as_f64;
    }
    if (vm->stack[vm->stack_size - 2].val.as_i64 == STRING) {
      ins_val.datatype = STRING;
      ins_val.val.as_str = vm->stack[vm->stack_size - 1].val.as_ptr;
    }
    sql_push((*ins_val_arr_tmp), ins_val);
    vm->stack_size -= 2;
    vm->pinst++;

    break;

  case INST_FROM_TABLES:;
    SQL_Table_List_n_Cursor *tab_list =
        calloc(1, sizeof(SQL_Table_List_n_Cursor));
    MemArea *mem = vm->stack[vm->stack_size - 1].val.as_ptr;
    for (size_t i = 0; i < vm->stack_size - 1; i++) {
      Node *node = (Node *)vm->stack[i].val.as_ptr;
      sql_push((*tab_list),
               ((SQL_Table_n_Cursor){
                   .cursor = 0,
                   .slotcnt = sql_get_table_slotcnt(node->token->str, mem),
                   .node = node,
               }

                ));
    }
    vm->stack_size = 0;
    vm->stack[vm->stack_size++].val.as_ptr = tab_list;
    vm->stack[vm->stack_size++].val.as_ptr = sql_cartesian_rowset_generator;

    vm->pinst++;

    break;

  case INST_GENERATOR:
    if (vm->stack_size < 5) {
      return INST_SIG_UNDERFOW;
    }
    SQL_Table_List_n_Cursor *table_list = vm->stack[0].val.as_ptr;
    vm->stack[5].val.as_ptr = calloc(1, sizeof(SQL_Unfiltered_Rowset));
    SQL_Unfiltered_Rowset **rowset =
        (SQL_Unfiltered_Rowset **)&vm->stack[5].val.as_ptr;
    HashMap *table_definition = vm->stack[3].val.as_ptr;
    MemArea *mem_tmp = vm->stack[2].val.as_ptr;
    void (*fptr)(SQL_Table_List_n_Cursor *, SQL_Unfiltered_Rowset **, HashMap *,
                 MemArea *) = vm->stack[1].val.as_ptr;
    fptr(table_list, rowset, table_definition, mem_tmp);
    vm->pinst++;
    break;

  case INST_FETCH_VAL:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    if (vm->stack[vm->stack_size - 1].tag != WORD_PTR) {
      return INST_SIG_ILLEGAL;
    }
    Node *dotexpr = vm->stack[vm->stack_size - 1].val.as_ptr;
    SQL_Unfiltered_Rowset *rowset_fetch_val = vm->stack[5].val.as_ptr;
    int cursor = 0;
    for (size_t i = 0; i < rowset_fetch_val->used; i++) {
      if (strcasecmp(rowset_fetch_val->items[i].tabname,
                     dotexpr->expr_lhs->token->str) == 0 &&
          strcasecmp(rowset_fetch_val->items[i].colname,
                     dotexpr->expr_rhs->token->str) == 0) {
        switch (rowset_fetch_val->items[i].type) {
        case SQL_TYPE_CHAR:
          vm->stack[vm->stack_size - 1].tag = WORD_STR;
          vm->stack[vm->stack_size - 1].val.as_ptr =
              &rowset_fetch_val->data[cursor];
          break;
        case SQL_TYPE_INT:
          vm->stack[vm->stack_size - 1].tag = WORD_I64;
          vm->stack[vm->stack_size - 1].val.as_i64 =
              *(int64_t *)&rowset_fetch_val->data[cursor];
          break;
        case SQL_TYPE_FLOAT:
          vm->stack[vm->stack_size - 1].tag = WORD_F64;
          vm->stack[vm->stack_size - 1].val.as_i64 =
              *(double *)&rowset_fetch_val->data[cursor];
          break;
        }
        break;
      }
      cursor += rowset_fetch_val->items[i].len;
    }
    vm->pinst++;
    break;

  case INST_PUSH_RES_NZ:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    {
      SQL_Filtered_Rowset *res_rowset = vm->stack[4].val.as_ptr;
      SQL_Unfiltered_Rowset *rowset = vm->stack[5].val.as_ptr;
      if (vm->stack[vm->stack_size - 1].val.as_u64) {
        sql_push((*res_rowset), (*rowset));
        free(rowset);
      } else {
        free(rowset->items);
        free(rowset->data);
        free(rowset->null_bytemap);
        free(rowset);
      }
    }
    vm->stack_size--;
    vm->pinst++;
    break;

  case INST_PUSH_SELECT_LIST:
    if (vm->stack_size < 2) {
      return INST_SIG_UNDERFOW;
    }
    SQL_Select_List *select_list = vm->stack[vm->stack_size - 2].val.as_ptr;
    Word *cur_word = &vm->stack[vm->stack_size - 1];
    sql_push((*select_list), (*cur_word));
    vm->stack_size--;
    vm->pinst++;

    break;

  case INST_SELECT_PROJECT:
    if (vm->stack_size < 6) {
      return INST_SIG_UNDERFOW;
    }
    SQL_Resultset *result = vm->stack[vm->stack_size - 1].val.as_ptr;
    SQL_Select_List *select_list_proj =
        vm->stack[vm->stack_size - 2].val.as_ptr;
    SQL_Filtered_Rowset *filtered_rowset =
        vm->stack[vm->stack_size - 3].val.as_ptr;
    HashMap *table_definition_proj = vm->stack[3].val.as_ptr;
    int result_rowset_size = 0;
    for (size_t i = 0; i < select_list_proj->used; i++) {
      // calculate resultset data size
      switch (select_list_proj->items[i].tag) {
      case WORD_PTR:
        Node *select_field = select_list_proj->items[i].val.as_ptr;
        if (select_field->expr_rhs->type.exacttype == MUL) {
          result_rowset_size += sql_get_table_tuplesize(
              select_field->expr_lhs->token->str, table_definition_proj);
        } else {
          SQL_Cartesian_Product_Layout type_n_size = sql_get_col_type_n_size(
              select_field->expr_lhs->token->str,
              select_field->expr_rhs->token->str, table_definition_proj);
          result_rowset_size += type_n_size.len;
        }
        break;
      case WORD_STR:
        result_rowset_size += strlen(select_list_proj->items[i].val.as_ptr) + 1;
        break;
      case WORD_I64:
        result_rowset_size += 8;
        break;
      case WORD_F64:
        result_rowset_size += 8;
        break;
      // case WORD_U64:
      //   result_rowset_size += 8;
      //   break;
      case WORD_NIL:
        break;
      }
    }

    result->data_size = result_rowset_size;
    //
    for (size_t i = 0; i < select_list_proj->used; i++) {
      switch (select_list_proj->items[i].tag) {
      case WORD_PTR:
        Node *select_field = select_list_proj->items[i].val.as_ptr;
        int table_def_idx = hashmap_find(table_definition_proj,
                                         select_field->expr_lhs->token->str);
        SQL_Col_Def_Array *col_def_arr =
            (SQL_Col_Def_Array *)table_definition_proj->items[table_def_idx]
                .pageno[0];
        if (select_field->expr_rhs->type.exacttype == MUL) {
          for (size_t j = 0; j < col_def_arr->used; j++) {
            switch (col_def_arr->items[j].type) {
            case STRING:
              sql_col_push((*result),
                           ((SQL_Cartesian_Product_Layout){
                               .tabname = select_field->expr_lhs->token->str,
                               .colname = col_def_arr->items[j].colname,
                               .len = col_def_arr->items[j].len,
                               .type = SQL_TYPE_CHAR}));
              break;
            case INTEGER:
              sql_col_push((*result),
                           ((SQL_Cartesian_Product_Layout){
                               .tabname = select_field->expr_lhs->token->str,
                               .colname = col_def_arr->items[j].colname,
                               .len = col_def_arr->items[j].len,
                               .type = SQL_TYPE_INT}));
              break;
            case FLOAT:
              sql_col_push((*result),
                           ((SQL_Cartesian_Product_Layout){
                               .tabname = select_field->expr_lhs->token->str,
                               .colname = col_def_arr->items[j].colname,
                               .len = col_def_arr->items[j].len,
                               .type = SQL_TYPE_FLOAT}));
              break;
            }
            //
          }
        } else {
          sql_col_push((*result),
                       ((SQL_Cartesian_Product_Layout){
                           .tabname = select_field->expr_lhs->token->str,
                           .colname = select_field->expr_rhs->token->str,
                           .len = sql_get_col_type_n_size(
                                      select_field->expr_lhs->token->str,
                                      select_field->expr_rhs->token->str,
                                      table_definition_proj)
                                      .len,
                           .type = sql_get_col_type_n_size(
                                       select_field->expr_lhs->token->str,
                                       select_field->expr_rhs->token->str,
                                       table_definition_proj)
                                       .type}));
        }
        break;
      case WORD_STR:
        sql_col_push((*result),
                     ((SQL_Cartesian_Product_Layout){
                         .colname = "string",
                         .len = strlen(select_list_proj->items[i].val.as_ptr),
                         .type = SQL_TYPE_CHAR}));
        break;
      case WORD_I64:
        sql_col_push((*result),
                     ((SQL_Cartesian_Product_Layout){.colname = "integer",
                                                     .len = 8,
                                                     .type = SQL_TYPE_INT}));
        break;
      case WORD_F64:
        sql_col_push((*result),
                     ((SQL_Cartesian_Product_Layout){.colname = "float",
                                                     .len = 8,
                                                     .type = SQL_TYPE_FLOAT}));
        break;
      // case WORD_U64:
      //   result_rowset_size += 8;
      //   break;
      case WORD_NIL:
        break;
      }
    }

    for (size_t i = 0; i < filtered_rowset->used; i++) {
      // fill result rowset with row value
      SQL_Result_Rowset rowset = {0};
      rowset.data = calloc(1, result_rowset_size);
      rowset.null_bytemap = calloc(1, result->col_used);
      int cursor = 0;
      int literal_idx = 0;
      for (size_t j = 0; j < result->col_used; j++) {
        if (result->col_items[j].tabname == NULL) {
          for (size_t k = literal_idx; k < select_list_proj->used; k++) {
            // fill in literal;
            if (select_list_proj->items[k].tag != WORD_PTR) {
              if (select_list_proj->items[k].tag == WORD_STR) {
                memcpy(rowset.data + cursor,
                       select_list_proj->items[k].val.as_ptr,
                       result->col_items[j].len);
              } else {
                memcpy(rowset.data + cursor,
                       &select_list_proj->items[k].val.as_u64,
                       result->col_items[j].len);
              }
              literal_idx = k + 1;
              break;
            }
          }
          cursor += result->col_items[j].len;
        } else {
          memcpy(rowset.data + cursor,
                 filtered_rowset->items[i].data +
                     sql_get_table_field_offset_in_filtered_rowset(
                         result->col_items[j].tabname,
                         result->col_items[j].colname,
                         &filtered_rowset->items[i]),
                 result->col_items[j].len);
          cursor += result->col_items[j].len;
        }
        rowset.null_bytemap[j] = sql_is_col_null(result->col_items[j].tabname,
                                                 result->col_items[j].colname,
                                                 filtered_rowset->items[i]);
      }
      sql_push((*result), rowset);
    }

    // for (size_t i = 0; i < filtered_rowset->used; i++) {
    //   sql_print_unfiltered_rowset(&filtered_rowset->items[i]);
    // }
    sql_print_resultset(result);

    //
    vm->pinst++;
    break;

  case INST_SUB_PREFIX:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
    switch (vm->stack[vm->stack_size - 1].tag) {
    case WORD_I64:
      vm->stack[vm->stack_size - 1].val.as_i64 =
          -vm->stack[vm->stack_size - 1].val.as_i64;
      break;
    case WORD_U64:
      vm->stack[vm->stack_size - 1].val.as_i64 =
          -vm->stack[vm->stack_size - 1].val.as_u64;
      vm->stack[vm->stack_size - 1].tag = WORD_I64;
      break;
    case WORD_F64:
      vm->stack[vm->stack_size - 1].val.as_f64 =
          -vm->stack[vm->stack_size - 1].val.as_f64;
      break;
    default:
      return INST_SIG_UNDERFOW;
      break;
    }
    vm->pinst++;
    break;

  case INST_ADD_PREFIX:
    if (vm->stack_size < 1) {
      return INST_SIG_UNDERFOW;
    }
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
    switch (vm.stack[i].tag) {
    case WORD_PTR:
      printf("[%3lu]:  %p\n", i, vm.stack[i].val.as_ptr);
      break;
    case WORD_STR:
      printf("[%3lu]:  %s\n", i, (char *)vm.stack[i].val.as_ptr);
      break;
    case WORD_I64:
      printf("[%3lu]:  %ld\n", i, vm.stack[i].val.as_i64);
      break;
    case WORD_F64:
      printf("[%3lu]:  %lf\n", i, vm.stack[i].val.as_f64);
      break;
    case WORD_U64:
      printf("[%3lu]:  %lu\n", i, vm.stack[i].val.as_u64);
      break;
    case WORD_NIL:
      printf("[%3lu]:  NULL\n", i);
      break;
    }
  }
}

void vm_dump_inst(VM_Instruction *program, int program_size) {
  puts("Program instruction");
  for (int i = 0; i < program_size; i++) {
    printf("[%3d]:"
           "%s ",
           i, vm_ins_as_cstr(program[i]));
    printf("(");
    switch (program[i].operand.tag) {
    case WORD_PTR:
      printf("%p", program[i].operand.val.as_ptr);
      break;
    case WORD_STR:
      printf("%s", (char *)program[i].operand.val.as_ptr);
      break;
    case WORD_I64:
      printf("%ld", program[i].operand.val.as_i64);
      break;
    case WORD_F64:
      printf("%lf", program[i].operand.val.as_f64);
      break;
    case WORD_U64:
      printf("%lu", program[i].operand.val.as_u64);
      break;
    case WORD_NIL:
      printf("NULL");
      break;
    }
    printf(")\n");
  }
}

void vm_dump_stack_n_inst(VM vm, VM_Instruction program[]) {
  puts("Stack:");
  for (size_t i = 0; i < vm.stack_size; i++) {
    switch (vm.stack[i].tag) {
    case WORD_PTR:
      printf("[%3lu]:  %p\n", i, vm.stack[i].val.as_ptr);
      break;
    case WORD_STR:
      printf("[%3lu]:  %s\n", i, (char *)vm.stack[i].val.as_ptr);
      break;
    case WORD_I64:
      printf("[%3lu]:  %ld\n", i, vm.stack[i].val.as_i64);
      break;
    case WORD_F64:
      printf("[%3lu]:  %lf\n", i, vm.stack[i].val.as_f64);
      break;
    case WORD_U64:
      printf("[%3lu]:  %lu\n", i, vm.stack[i].val.as_u64);
      break;
    case WORD_NIL:
      printf("[%3lu]:  NULL\n", i);
      break;
    }
  }
  puts("Instruction:");
  printf(
      "[%3lu]:"
      "  "
      "%s ",
      vm.pinst,
      vm_ins_as_cstr(program[vm.pinst])); // TODO: if %s is null, then UB, but
                                          // gcc works because printf can handle
                                          // this, but not optimized puts
  printf("(");
  switch (program[vm.pinst].operand.tag) {
  case WORD_PTR:
    printf("%p", program[vm.pinst].operand.val.as_ptr);
    break;
  case WORD_STR:
    printf("%s", (char *)program[vm.pinst].operand.val.as_ptr);
    break;
  case WORD_I64:
    printf("%ld", program[vm.pinst].operand.val.as_i64);
    break;
  case WORD_F64:
    printf("%lf", program[vm.pinst].operand.val.as_f64);
    break;
  case WORD_U64:
    printf("%lu", program[vm.pinst].operand.val.as_u64);
    break;
  case WORD_NIL:
    printf("NULL");
    break;
  }
  printf(")\n");
}

const char *vm_word_type_as_cstr(Word_Tag tag) {
  switch (tag) {
  case WORD_U64:
    return "WORD_U64";
    break;
  case WORD_F64:
    return "WORD_F64";
    break;
  case WORD_I64:
    return "WORD_I64";
    break;
  case WORD_PTR:
    return "WORD_PTR";
    break;
  case WORD_STR:
    return "WORD_STR";
    break;
  case WORD_NIL:
    return "WORD_NIL";
    break;
  }
  return "";
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
  case INST_ADD_PREFIX:
    return "INST_ADD_PREFIX";
  case INST_SUB_PREFIX:
    return "INST_SUB_PREFIX";
  case INST_GENERATOR:
    return "INST_GENERATOR";
  case INST_SELECT_PROJECT:
    return "INST_SELECT_PROJECT";
  case INST_PUSH_SELECT_LIST:
    return "INST_PUSH_SELECT_LIST";
  case INST_PUSH_RES_NZ:
    return "INST_PUSH_RES_NZ";
  case INST_HLT_IFZ:
    return "INST_HLT_IFZ";
  case INST_FETCH_VAL:
    return "INST_FETCH_VAL";
  case INST_CREATE_TABLE:
    return "INST_CREATE_TABLE";
  case INST_DEFINE_COLUMN:
    return "INST_DEFINE_COLUMN";
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
  case INST_FROM_TABLES:
    return "INST_FROM_TABLES";
  case INST_NEQ:
    return "INST_NEQ";
  case INST_LEQ:
    return "INST_LEQ";
  case INST_GEQ:
    return "INST_GEQ";
  case INST_GT:
    return "INST_GT";
  case INST_LT:
    return "INST_LT";
  case INST_OR:
    return "INST_OR";
  case INST_AND:
    return "INST_AND";
  case INST_NOT:
    return "INST_NOT";
  }
  return "";
}

VM_Instruction vm_push(Word operand) {
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
  return (VM_Instruction){.inst_type = INST_JMP,
                          .operand = {.tag = WORD_I64, .val.as_i64 = pinst}};
}
VM_Instruction vm_hlt() { return (VM_Instruction){.inst_type = INST_HLT}; }

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

VM_Instruction vm_not() {
  return (VM_Instruction){
      .inst_type = INST_NOT,
  };
}
VM_Instruction vm_and() {
  return (VM_Instruction){
      .inst_type = INST_AND,
  };
}
VM_Instruction vm_or() {
  return (VM_Instruction){
      .inst_type = INST_OR,
  };
}
VM_Instruction vm_neq() {
  return (VM_Instruction){
      .inst_type = INST_NEQ,
  };
}
VM_Instruction vm_lt() {

  return (VM_Instruction){
      .inst_type = INST_LT,
  };
}
VM_Instruction vm_leq() {
  return (VM_Instruction){
      .inst_type = INST_LEQ,
  };
}
VM_Instruction vm_gt() {
  return (VM_Instruction){
      .inst_type = INST_GT,
  };
}
VM_Instruction vm_geq() {
  return (VM_Instruction){
      .inst_type = INST_GEQ,
  };
}

void vm_dump_word(Word word) {
  switch (word.tag) {
  case WORD_PTR:
    printf("%p", word.val.as_ptr);
    break;
  case WORD_STR:
    printf("%s", (char *)word.val.as_ptr);
    break;
  case WORD_I64:
    printf("%ld", word.val.as_i64);
    break;
  case WORD_F64:
    printf("%lf", word.val.as_f64);
    break;
  case WORD_U64:
    printf("%lu", word.val.as_u64);
    break;
  case WORD_NIL:
    printf("NULL");
    break;
  }
  puts("");
}

VM_Instruction vm_jnz(size_t pinst) {
  return (VM_Instruction){.inst_type = INST_JNZ,
                          .operand = {.val.as_i64 = pinst}};
}
VM_Instruction vm_dup(size_t pstack) {
  return (VM_Instruction){.inst_type = INST_DUP,
                          .operand = {.val.as_i64 = pstack}};
}
VM_Instruction vm_nop() { return (VM_Instruction){0}; }
VM_Instruction vm_swap(size_t pstack) {
  return (VM_Instruction){.inst_type = INST_SWAP,
                          .operand = {.val.as_i64 = pstack}};
}
VM_Instruction vm_create_table() {
  return (VM_Instruction){.inst_type = INST_CREATE_TABLE};
}
VM_Instruction vm_define_column() {
  return (VM_Instruction){.inst_type = INST_DEFINE_COLUMN};
}

VM_Instruction vm_sql_from_table() {
  return (VM_Instruction){.inst_type = INST_FROM_TABLES};
}

VM_Instruction vm_gen() {
  return (VM_Instruction){.inst_type = INST_GENERATOR};
}

VM_Instruction vm_fetchval() {
  return (VM_Instruction){.inst_type = INST_FETCH_VAL};
}
VM_Instruction vm_hlt_ifz() {
  return (VM_Instruction){.inst_type = INST_HLT_IFZ};
}
VM_Instruction vm_push_res_nz() {
  return (VM_Instruction){.inst_type = INST_PUSH_RES_NZ};
}
VM_Instruction vm_push_sel_list() {
  return (VM_Instruction){.inst_type = INST_PUSH_SELECT_LIST};
}
VM_Instruction vm_select_project() {
  return (VM_Instruction){.inst_type = INST_SELECT_PROJECT};
}
VM_Instruction vm_add_prefix() {
  return (VM_Instruction){.inst_type = INST_ADD_PREFIX};
}
VM_Instruction vm_sub_prefix() {
  return (VM_Instruction){.inst_type = INST_SUB_PREFIX};
}
