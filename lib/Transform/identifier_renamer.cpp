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
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

static cl::opt<bool> disableRenamer(
    "disableRenamer", cl::init(false),
    cl::desc("Disable Rename pass regardless. Useful when used in -OX mode."));

bool IdentifierRenamer::runOnModule(Module &M) {
  if (disableRenamer)
    return false;

  // Rename globals if possible
  for (auto G = M.global_begin(), GEnd = M.global_end(); G != GEnd; ++G) {
    GlobalValue::LinkageTypes linkage = G->getLinkage();
    if (linkage == GlobalValue::InternalLinkage ||
        linkage == GlobalValue::PrivateLinkage) {
      DEBUG(errs() << "\tRemoving globals\n");
      G->setName("");
    }
  }

  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

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
  }

  return true;
}

char IdentifierRenamer::ID = 0;
static RegisterPass<IdentifierRenamer>
    X("identifier-renamer", "Remove identifiers and function names if possible",
      false, false);
