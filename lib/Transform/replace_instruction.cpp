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
#include "Transform/opaque_predicate.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <climits>
#include <utility>
#include <vector>

static cl::opt<std::string> replaceSeed(
    "replaceSeed", cl::init(""),
    cl::desc("Seed for random number generator. Defaults to system time"));

namespace {
static const unsigned intOpSize = 13;
static unsigned intOps[intOpSize] = {
  Instruction::Add, Instruction::Sub, Instruction::Mul, Instruction::UDiv,
  Instruction::SDiv, Instruction::URem, Instruction::SRem, Instruction::Shl,
  Instruction::LShr, Instruction::AShr, Instruction::And, Instruction::Or,
  Instruction::Xor
};

static const unsigned floatOpsSize = 5;
static unsigned floatOps[floatOpsSize] = { Instruction::FAdd, Instruction::FSub,
                                           Instruction::FMul, Instruction::FDiv,
                                           Instruction::FRem };
}

bool ReplaceInstruction::runOnBasicBlock(BasicBlock &block) {
  // Check that block is unreachable
  if (!OpaquePredicate::isBasicBlockUnreachable(block)) {
    return false;
  }
  DEBUG(errs() << "Unreachable Block: " << block.getName() << "\n");

  std::minstd_rand engine;
  std::uniform_int_distribution<unsigned> distribution;
  // Seed engine and create distribution
  if (!replaceSeed.empty()) {
    std::seed_seq seed(replaceSeed.begin(), replaceSeed.end());
    engine.seed(seed);
  } else {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine.seed(seed);
  }

  bool hasBeenModified = false;
  std::vector<std::pair<Instruction *, Instruction *> > replacements;

  for (Instruction &inst : block) {
    DEBUG(errs() << "\t" << inst << "\n");
    if (inst.isTerminator()) {
      DEBUG(errs() << "\t\tSkipping terminator\n");
      continue;
    }

    if (inst.isBinaryOp()) {
      DEBUG(errs() << "\t\tBinary operator\n");

      unsigned opcode = inst.getOpcode();
      unsigned newOpcode = opcode;

      if (std::find(intOps, intOps + intOpSize, opcode) !=
          (intOps + intOpSize)) {
        DEBUG(errs() << "\t\tInteger operator\n");
        // Integer
        while (newOpcode == opcode) {
          unsigned choice = distribution(engine) % intOpSize;
          newOpcode = intOps[choice];
        }
        // Create new instruction
      } else if (std::find(floatOps, floatOps + floatOpsSize, opcode) !=
                 (floatOps + floatOpsSize)) {
        // Float
        DEBUG(errs() << "\t\tFloating point operator\n");
        while (newOpcode == opcode) {
          unsigned choice = distribution(engine) % floatOpsSize;
          newOpcode = floatOps[choice];
        }
      } else {
        llvm_unreachable("Unknown binary operator");
      }

      // Create new instruction
      BinaryOperator *newInst =
          BinaryOperator::Create((Instruction::BinaryOps)newOpcode,
                                 inst.getOperand(0), inst.getOperand(1));
      DEBUG(errs() << "\t\tReplacing with new insturction: " << *newInst
                   << "\n");
      replacements.push_back(std::make_pair(&inst, (Instruction *)newInst));
    }
  }

  if (replacements.empty()) {
    // Do some warning
    errs() << "WARNING: Unable to find suitable replacements for unreachable "
              "block -- will be optimised away\n";
  } else {
    for (auto &pair : replacements) {
      ReplaceInstWithInst(pair.first, pair.second);
      hasBeenModified = true;
    }
  }
  return hasBeenModified;
}

char ReplaceInstruction::ID = 0;
static RegisterPass<ReplaceInstruction> X(
    "replace-instruction",
    "Replace instructions on blocks marked as unreachable. This pass should be "
    "scheduled after the Bogus Control Flow pass or it will not do anything.",
    false, false);
