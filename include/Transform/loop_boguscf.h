//=== loop_boguscf.h - Loop Bogus Control Flow     ===========================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LOOP_BOGUSCF_H
#define LOOP_BOGUSCF_H
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
using namespace llvm;

struct LoopBogusCF : public LoopPass {
  static char ID;

  LoopBogusCF() : LoopPass(ID) {}
  virtual bool runOnLoop(Loop *loop, LPPassManager &LPM);
};

#endif
