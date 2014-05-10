//=== replace_instruction.h - Replace instructions in unreachable blocks =====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "replace-instruction"
#include "Transform/replace_instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

bool ReplaceInstruction::runOnBasicBlock(BasicBlock &block) { return false; }

char ReplaceInstruction::ID = 0;
static RegisterPass<ReplaceInstruction> X(
    "replace-instruction",
    "Replace instructions on blocks marked as unreachable. This pass should be "
    "scheduled after the Bogus Control Flow pass or it will not do anything.",
    false, false);
