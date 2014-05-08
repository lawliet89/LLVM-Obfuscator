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
#include "Transform/opaque_predicate.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
// Schedue the passes
// http://homes.cs.washington.edu/~bholt/posts/llvm-quick-tricks.html
static RegisterStandardPasses Y(PassManagerBuilder::EP_OptimizerLast,
                                [](const PassManagerBuilder &,
                                   PassManagerBase &PM) {

  // First batch of passes are trivial passes and should be run first to
  // "maximise confusion" that the later passes will introduce
  PM.add(new Copy());
  PM.add(new InlineFunctionPass());

  // Second batch of passes deal with introducing new control flow paths
  // These passes will insert stub 1.00 == 1.00 branches
  // which will be cleaned up by the OpaquePredicate pass
  // OpaquePredicate pass MUST be run before Flatten Pass because Flatten
  // will REMOVE the original branch instructions
  PM.add(new BogusCF());
  PM.add(createLoopSimplifyPass());
  PM.add(new LoopBogusCF());
  PM.add(new OpaquePredicate());

  // Flatten the control flow
  PM.add(new Flatten());

  // Clean ups
  PM.add(new CleanupPass()); // Remove stray metadata left over from passes
  PM.add(createPromoteMemoryToRegisterPass()); // Fix PHI demotions
  PM.add(createCFGSimplificationPass()); // Further cleanups

  PM.add(new IdentifierRenamer());
});
