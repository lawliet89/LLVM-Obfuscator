//=== inline_function.h - INline funciton call Obfuscation Pass -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef INLINE_FUNCTION_H
#define INLINE_FUNCTION_H

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include <random>

using namespace llvm;

struct InlineFunctionPass : public FunctionPass {
  static char ID;
  std::mt19937_64 engine;
  std::bernoulli_distribution trial;

  InlineFunctionPass() : FunctionPass(ID) {}

  // Initialise and check options
  virtual bool doInitialization(Module &M);
  virtual bool runOnFunction(Function &F);
};
#endif
