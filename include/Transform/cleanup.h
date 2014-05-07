//=== cleanup.h - Obfuscation Cleanup Pass ===================================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef CLEANUP_H
#define CLEANUP_H
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
using namespace llvm;

struct CleanupPass : public FunctionPass {
  static char ID;

  CleanupPass() : FunctionPass(ID) {}
  virtual bool runOnFunction(Function &F);
};

#endif
