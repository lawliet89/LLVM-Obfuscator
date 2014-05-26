//=== flatten.h   - Flatten Control Flow Pass  ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef FLATTEN_H
#define FLATTEN_H

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include <random>

using namespace llvm;

struct Flatten : public FunctionPass {
  static char ID;
  std::mt19937_64 engine;
  std::bernoulli_distribution trial;
  StringRef metaKindName;

  Flatten() : FunctionPass(ID), metaKindName("FlattenSwitch") {}

  inline Value *findBlock(LLVMContext &context,
                          std::vector<BasicBlock *> &blocks, BasicBlock *block);
  virtual bool doInitialization(Module &M);
  virtual bool runOnFunction(Function &F);
  static bool isEligible(Function &F);
};

#endif
