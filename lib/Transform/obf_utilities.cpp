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
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <vector>

namespace {
StringRef getMetaKindName(ObfUtils::ObfType type) {
  switch (type) {
  case ObfUtils::BogusCFObf:
    return StringRef("obf_boguscf");
  case ObfUtils::FlattenObf:
    return StringRef("obf_flatten");
  default:
    llvm_unreachable("Unknown obfuscation type");
  }
}
};

namespace ObfUtils {
// Tag a function as "obfuscated"
void tagFunction(Function &F, ObfType type) {
  LLVMContext &context = F.getContext();
  StringRef metaKindName = getMetaKindName(type);
  MDNode *metaNode = MDNode::get(context, MDString::get(context, metaKindName));
  unsigned metaKind = context.getMDKindID(metaKindName);

  // Get first instruction
  Instruction *first = (Instruction *)(F.getEntryBlock().begin());
  first->setMetadata(metaKind, metaNode);
}

// Check if a function has been tagged as obfuscated of type
bool checkFunctionTagged(Function &F, ObfType type) {
  LLVMContext &context = F.getContext();
  StringRef metaKindName = getMetaKindName(type);
  unsigned metaKind = context.getMDKindID(metaKindName);
  // Get first instruction
  Instruction *first = (Instruction *)(F.getEntryBlock().begin());
  return bool(first->getMetadata(metaKind));
}

void promoteAllocas(Function &F, DominatorTree &DT) {
  std::vector<AllocaInst *> allocas;
  for (auto &block : F) {
    for (Instruction &inst : block) {
      if (AllocaInst *alloca = dyn_cast<AllocaInst>(&inst)) {
        if (isAllocaPromotable(alloca)) {
          allocas.push_back(alloca);
        }
      }
    }
  }

  DT.getBase().recalculate(F);
  PromoteMemToReg(allocas, DT);
}
};
