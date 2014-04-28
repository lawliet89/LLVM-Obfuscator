//=== obf_utilities.h - Utilities for Obfuscation passes  -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Opaque predicates are based on the equations given in the paper at
// http://crypto.cs.mcgill.ca/~garboit/sp-paper.pdf

#include "Transform/obf_utilities.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"

namespace {
StringRef metaKindName = "obf_mark";
};

namespace ObfUtils {
// Tag a function as "obfuscated"
void tagFunction(Function &F) {
  LLVMContext &context = F.getContext();
  MDNode *metaNode = MDNode::get(context, MDString::get(context, metaKindName));
  unsigned metaKind = context.getMDKindID(metaKindName);

  // Get first instruction
  Instruction *first = (Instruction *)(F.getEntryBlock().begin());
  first->setMetadata(metaKind, metaNode);
}

// Check if a function has been tagged as obfuscated
bool checkFunctionTagged(Function &F) {
  LLVMContext &context = F.getContext();
  unsigned metaKind = context.getMDKindID(metaKindName);
  // Get first instruction
  Instruction *first = (Instruction *)(F.getEntryBlock().begin());
  return bool(first->getMetadata(metaKind));
}
};
