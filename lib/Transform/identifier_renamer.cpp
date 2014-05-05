//=== identifier_renamer.cpp - Trivial identifier renaming ===================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "copy"
#include "Transform/identifier_renamer.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

bool IdentifierRenamer::runOnBasicBlock(BasicBlock &BB) {
  BB.setName("");
  for (Instruction &inst : BB) {
    inst.setName("");
  }
  return true;
}

char IdentifierRenamer::ID = 0;
static RegisterPass<IdentifierRenamer>
X("identifier-renamer", "Remove names of Values", false, false);
