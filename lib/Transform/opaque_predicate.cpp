//=== opaque_predicate.h - Manufactures opaque predicates -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Opaque predicates are based on the equations given in the paper at
// http://crypto.cs.mcgill.ca/~garboit/sp-paper.pdf

#include "Transform/opaque_predicate.h"
#include "llvm/IR/Constants.h"
#include <cassert>
using namespace llvm;

namespace OpaquePredicate {
std::vector<GlobalVariable *> prepareModule(Module &M, unsigned number) {
  assert(number >= 2 && "Opaque Predicates need at least 2 global variables");
  std::vector<GlobalVariable *> globals(number);
  for (unsigned i = 0; i < number; ++i) {
    Value *zero = ConstantInt::get(Type::getInt32Ty(M.getContext()),
                                     0, true);
    GlobalVariable *global = new GlobalVariable(M,
                                    Type::getInt32Ty(M.getContext()), false,
                                    GlobalValue::CommonLinkage,
                                    (Constant * ) zero);
    globals[i] = global;
  }
  return globals;
}
};
