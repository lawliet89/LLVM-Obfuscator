//=== identifier_renamer.h - Trivial identifier renaming =====================//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef IDENTIFIER_RENAMER_H
#define IDENTIFIER_RENAMER_H
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
using namespace llvm;

struct IdentifierRenamer : public ModulePass {
  static char ID;

  IdentifierRenamer() : ModulePass(ID) {}
  virtual bool runOnModule(Module &M);
};

#endif
