//=== replace_instruction.h - Replace instructions in unreachable blocks =====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef REPLACE_INSTRUCTION_H
#define REPLACE_INSTRUCTION_H
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
using namespace llvm;

struct ReplaceInstruction : public BasicBlockPass {
  static char ID;

  ReplaceInstruction() : BasicBlockPass(ID) {}
  virtual bool runOnBasicBlock (BasicBlock &BB);
};

#endif
