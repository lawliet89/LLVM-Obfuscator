//=== schedule.cpp - Schedule the passes------ ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "Transform/boguscf.h"
#include "Transform/cleanup.h"
#include "Transform/copy.h"
#include "Transform/flatten.h"
#include "Transform/identifier_renamer.h"
#include "Transform/inline_function.h"
#include "Transform/loop_boguscf.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
// Schedue the passes
// http://homes.cs.washington.edu/~bholt/posts/llvm-quick-tricks.html
static RegisterStandardPasses Y(PassManagerBuilder::EP_OptimizerLast,
                                [](const PassManagerBuilder &,
                                   PassManagerBase &PM) {
  PM.add(new Copy());
  PM.add(new InlineFunctionPass());
  PM.add(new BogusCF());
  PM.add(new Flatten());
  PM.add(new IdentifierRenamer());
  PM.add(new LoopBogusCF());
  // Clean ups
  PM.add(new CleanupPass());
  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(createCFGSimplificationPass());
});
