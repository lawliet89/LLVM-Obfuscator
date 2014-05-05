//=== identifier_renamer.cpp - Trivial identifier renaming ===================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "renamer"
#include "Transform/identifier_renamer.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

bool IdentifierRenamer::runOnFunction(Function &F) {
  if (F.isDeclaration())
    return false;

  DEBUG(errs() << "Renamer: Function '" << F.getName() << "'\n");

  // Check linkage of function and rename if internal/private
  GlobalValue::LinkageTypes linkage = F.getLinkage();
  if (linkage == GlobalValue::InternalLinkage ||
      linkage == GlobalValue::PrivateLinkage) {
    DEBUG(errs() << "\tRemoving function name\n");
    F.setName("");
  }

  DEBUG(errs() << "\tRemoving names of BasicBlocks and Values\n");
  for (auto &block : F) {
    block.setName("");
    for (Instruction &inst : block) {
      inst.setName("");
    }
  }

  return true;
}

char IdentifierRenamer::ID = 0;
static RegisterPass<IdentifierRenamer>
X("identifier-renamer", "Remove identifiers and function names if possible",
  false, false);
