//=== obf_utilities.h - Manufactures opaque predicates -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Opaque predicates are based on the equations given in the paper at
// http://crypto.cs.mcgill.ca/~garboit/sp-paper.pdf

#ifndef OBF_UTILITIES_H
#define OBF_UTILITIES_H

#include "llvm/IR/Function.h"
#include "llvm/Analysis/Dominators.h"
using namespace llvm;

namespace ObfUtils {
  enum ObfType {
    BogusCFObf,
    FlattenObf,
    CopyObf
  };

  // Tag a function as "obfuscated" - this can be useful for mutually exclusive
  // obfuscation passes
  void tagFunction(Function &F, ObfType type);

  // Check if a function has been tagged as obfuscated
  bool checkFunctionTagged(Function &F, ObfType type);

  // Promote all allocas to PHO, if possible
  void promoteAllocas(Function &F, DominatorTree &DT);
};

#endif
