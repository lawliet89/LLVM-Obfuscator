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
using namespace llvm;

namespace ObfUtils {
  // Tag a function as "obfuscated"
  void tagFunction(Function &F);

  // Check if a function has been tagged as obfuscated
  bool checkFunctionTagged(Function &F);
};

#endif
