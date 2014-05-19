//=== boguscf.h - Bogus Control Flow Obfuscation Pass -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef BOGUSCF_H
#define BOGUSCF_H

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include <random>

using namespace llvm;

struct BogusCF : public ModulePass {
  static char ID;
  std::mt19937_64 engine;
  std::bernoulli_distribution trial;

  BogusCF() : ModulePass(ID) {}

  // Initialise and check options
  virtual bool runOnModule(Module &M);
  bool runOnFunction(Function &F);

  // Check to see if a function is eligible for bogus CF processing
  static bool isEligible(Function &F);
};
#endif
