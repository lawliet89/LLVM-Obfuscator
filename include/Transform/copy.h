//=== copy.h - Copy function pass============  ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef COPY_H
#define COPY_H

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include <random>
using namespace llvm;

struct Copy : public ModulePass {
  static char ID;
  std::minstd_rand engine;
  std::bernoulli_distribution trial;

  Copy() : ModulePass(ID) {}
  virtual bool runOnModule(Module &M);
};

#endif
