//=== metrics.h - Potency analysis metrics ===================================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef METRICS_H
#define METRICS_H
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/LoopInfo.h"
using namespace llvm;

struct Metrics : public ModulePass {
  static char ID;

  Metrics() : ModulePass(ID) {}
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &Info) const {
    Info.setPreservesAll();
    Info.addRequired<LoopInfo>();
  }

private:
  unsigned calculateNest(BasicBlock &BB, LoopInfo &loopInfo);
};
#endif
