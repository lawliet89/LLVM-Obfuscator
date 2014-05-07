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
  DEBUG(errs() << "LoopBogusCF: Dumping loop info\n");
  DEBUG(loop->dump());

  BasicBlock *header = loop->getHeader();

  BranchInst *branch = dyn_cast<BranchInst>(header->getTerminator());
  if (!branch || !branch->isConditional()) {
    DEBUG(errs() << "\t Not trivial loop -- skipping\n");
    return false;
  }

  if (!loop->isLoopSimplifyForm()) {
    DEBUG(errs() << "\t Not simplified loop -- skipping\n");
    return false;
  }

  BasicBlock *exit = loop->getUniqueExitBlock();
  if (!exit) {
    DEBUG(errs() << "\t No unique exit block -- skipping\n");
    return false;
  }

  DEBUG(errs() << "\tCreating dummy block\n");
  LoopInfo &info = getAnalysis<LoopInfo>();
  // Split header block
  BasicBlock *dummy = header->splitBasicBlock(header->getTerminator());
  loop->addBasicBlockToLoop(dummy, info.getBase());

  if (branch->getSuccessor(1) == exit) {
    // True dummy predicate

  } else {
    // False dummy predicate
  }

  return true;
}

void LoopBogusCF::getAnalysisUsage (AnalysisUsage &AU) const {
  AU.addRequired<LoopInfo>();
}

char LoopBogusCF::ID = 0;
static RegisterPass<LoopBogusCF> X("loop-boguscf",
                                   "Insert opaque predicate to loop headers",
                                   false, false);
