//=== cleanup.cpp - Obfuscation Cleanup Pass =================================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "renamer"
#include "Transform/cleanup.h"
#include "Transform/obf_utilities.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

bool CleanupPass::runOnFunction(Function &F) {
  bool hasBeenModified = false;
  for (auto &block : F) {
    for (auto &inst : block) {
      hasBeenModified |=
          ObfUtils::removeTagIfExists(inst, ObfUtils::FlattenObf);
      hasBeenModified |=
          ObfUtils::removeTagIfExists(inst, ObfUtils::BogusCFObf);
      hasBeenModified |=
          ObfUtils::removeTagIfExists(inst, ObfUtils::CopyObf);
    }
  }
  return hasBeenModified;
}

char CleanupPass::ID = 0;
static RegisterPass<CleanupPass>
    X("cleanup", "Cleanup Residues left over by obfuscation passes", false,
      false);
