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
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

STATISTIC(NumLoops, "Number of loops inspected");
STATISTIC(NumLoopsObf, "Number of loops obfuscated");

static cl::opt<bool> disableLoopBcf(
    "disableLoopBcf", cl::init(false),
    cl::desc(
        "Disable Loop BCF pass regardless. Useful when used in -OX mode."));

bool LoopBogusCF::runOnLoop(Loop *loop, LPPassManager &LPM) {
  if (disableLoopBcf)
    return false;

  ++NumLoops;
  DEBUG(errs() << "LoopBogusCF: Dumping loop info\n");
  // DEBUG(loop->dump());

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

  BasicBlock *exitBlock = loop->getUniqueExitBlock();
  if (!exitBlock) {
    DEBUG(errs() << "\t No unique exit block -- skipping\n");
    return false;
  }

  if (branch->getSuccessor(0) != exitBlock &&
      branch->getSuccessor(1) != exitBlock) {
    DEBUG(errs() << "\t Not trivial loop -- skipping\n");
    return false;
  }

  ++NumLoopsObf;
  // DEBUG(header->getParent()->viewCFG());

  DEBUG(errs() << "\tCreating dummy block\n");
  LoopInfo &info = getAnalysis<LoopInfo>();
  // Split header block
  BasicBlock *dummy = header->splitBasicBlock(header->getTerminator());
  loop->addBasicBlockToLoop(dummy, info.getBase());

  BasicBlock *trueBlock, *falseBlock = exitBlock;

  if (branch->getSuccessor(0) == exitBlock) {
    trueBlock = branch->getSuccessor(1);
    branch->setSuccessor(1, dummy);
  } else {
    trueBlock = branch->getSuccessor(0);
    branch->setSuccessor(0, dummy);
  }

  branch->moveBefore(header->getTerminator());
  header->getTerminator()->eraseFromParent();

  OpaquePredicate::createStub(dummy, trueBlock, falseBlock,
                              OpaquePredicate::PredicateTrue, false);

  // DEBUG(header->getParent()->viewCFG());

  return true;
}

void LoopBogusCF::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfo>();
}

char LoopBogusCF::ID = 0;
static RegisterPass<LoopBogusCF> X("loop-boguscf",
                                   "Insert opaque predicate to loop headers",
                                   false, false);
