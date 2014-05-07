//=== loop_boguscf.hcpp- Loop Bogus Control Flow     =========================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "loop_boguscf"
#include "Transform/loop_boguscf.h"
#include "Transform/opaque_predicate.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

bool LoopBogusCF::runOnLoop(Loop *loop, LPPassManager &LPM) {
  return false;
}

char LoopBogusCF::ID = 0;
static RegisterPass<LoopBogusCF>
    X("loop-boguscf", "Insert opaque predicate to loop headers", false,
      false);
