//=== schedule.cpp - Schedule the passes------ ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "Transform/flatten.h"
#include "Transform/boguscf.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

// Schedue the passes
// http://homes.cs.washington.edu/~bholt/posts/llvm-quick-tricks.html
static RegisterStandardPasses Y(PassManagerBuilder::EP_OptimizerLast,
                                [](const PassManagerBuilder &,
                                   PassManagerBase &PM) {
  PM.add(new Flatten());
  PM.add(new BogusCF());
});
