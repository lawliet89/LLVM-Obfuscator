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
#include "Transform/obf_utilities.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include <random>
using namespace llvm;

struct Copy : public ModulePass {
  static char ID;
  std::mt19937_64 engine;
  std::bernoulli_distribution trial;

  Copy() : ModulePass(ID) {}
  virtual bool runOnModule(Module &M);

  // Tag this function as "must obfuscate of type"
  static void tagFunction(Function &F, ObfUtils::ObfType type);
  static bool isFunctionTagged(Function &F, ObfUtils::ObfType type);

private:
  static StringRef obfString(ObfUtils::ObfType type);
};

#endif
