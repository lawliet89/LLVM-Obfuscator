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
#define DEBUG_TYPE "utilities"
#include "Transform/obf_utilities.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Support/Debug.h"
#include <vector>

namespace {
StringRef getMetaKindName(ObfUtils::ObfType type) {
  switch (type) {
  case ObfUtils::BogusCFObf:
    return StringRef("obf_boguscf");
  case ObfUtils::FlattenObf:
    return StringRef("obf_flatten");
  case ObfUtils::CopyObf:
    return StringRef("obf_copy");
  default:
    llvm_unreachable("Unknown obfuscation type");
  }
}
};

namespace ObfUtils {
void tagFunction(Function &F, ObfType type, ArrayRef<Value *> values) {
  LLVMContext &context = F.getContext();
  StringRef metaKindName = getMetaKindName(type);
  MDNode *metaNode = MDNode::get(context, values);
  unsigned metaKind = context.getMDKindID(metaKindName);

  // Get first instruction
  Instruction *first = (Instruction *)(F.getEntryBlock().begin());
  first->setMetadata(metaKind, metaNode);
}

// Tag a function as "obfuscated"
void tagFunction(Function &F, ObfType type) {
  return tagFunction(F, type,
                     MDString::get(F.getContext(), getMetaKindName(type)));
}

// Check if a function has been tagged as obfuscated of type
MDNode *checkFunctionTagged(Function &F, ObfType type) {
  LLVMContext &context = F.getContext();
  StringRef metaKindName = getMetaKindName(type);
  unsigned metaKind = context.getMDKindID(metaKindName);
  // Get first instruction
  Instruction *first = (Instruction *)(F.getEntryBlock().begin());
  return first->getMetadata(metaKind);
}

void promoteAllocas(Function &F, DominatorTree &DT) {
  DEBUG(errs() << "PromoteAllocas: Function " << F.getName() << "\n");
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
  DEBUG(errs() << "\tPromoting " << allocas.size() << " allocas\n");
  DT.getBase().recalculate(F);
  PromoteMemToReg(allocas, DT);
}

bool removeTagIfExists(Instruction &I, ObfType type) {
  LLVMContext &context = I.getContext();
  StringRef metaKindName = getMetaKindName(type);
  unsigned metaKind = context.getMDKindID(metaKindName);
  if (I.getMetadata(metaKind)) {
    I.setMetadata(metaKind, nullptr);
    return true;
  } else {
    return false;
  }
}
};
