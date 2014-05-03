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

struct BogusCF : public FunctionPass {
  static char ID;
  std::minstd_rand engine;
  std::bernoulli_distribution trial;
  StringRef metaKindName;

  BogusCF() : FunctionPass(ID), metaKindName("opaqueStub") {}

  // Initialise and check options
  virtual bool doInitialization(Module &M);
  virtual bool runOnFunction(Function &F);
  virtual bool doFinalization(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &Info) const;

  // Check to see if a function is eligible for bogus CF processing
  static bool isEligible(Function &F);
};
#endif
