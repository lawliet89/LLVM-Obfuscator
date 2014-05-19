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
#include "llvm/ADT/Statistic.h"
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

STATISTIC(NumInstReplaced, "Number of instructions replaced");
STATISTIC(NumUnreachableBlocks, "Number of unreachable basic blocks");
STATISTIC(NumUnviableBlocks, "Number of unreachable basic blocks with no "
                             "viable instruction replacements");

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

static const unsigned intCompareSize = 10;
static unsigned intCompare[intCompareSize] = {
  CmpInst::ICMP_EQ, CmpInst::ICMP_NE, CmpInst::ICMP_UGT, CmpInst::ICMP_UGE,
  CmpInst::ICMP_ULT, CmpInst::ICMP_ULE, CmpInst::ICMP_SGT, CmpInst::ICMP_SGE,
  CmpInst::ICMP_SLT, CmpInst::ICMP_SLE
};

static const unsigned floatCompareSize = 14;
static unsigned floatCompare[floatCompareSize] = {
  CmpInst::FCMP_OEQ, CmpInst::FCMP_OGT, CmpInst::FCMP_OGE, CmpInst::FCMP_OLT,
  CmpInst::FCMP_OLE, CmpInst::FCMP_ONE, CmpInst::FCMP_ORD, CmpInst::FCMP_UNO,
  CmpInst::FCMP_UEQ, CmpInst::FCMP_UGT, CmpInst::FCMP_UGE, CmpInst::FCMP_ULT,
  CmpInst::FCMP_ULE, CmpInst::FCMP_UNE
};
}

bool ReplaceInstruction::runOnBasicBlock(BasicBlock &block) {
  // Check that block is unreachable
  if (!OpaquePredicate::isBasicBlockUnreachable(block)) {
    return false;
  }
  DEBUG(errs() << "Unreachable Block: " << block.getName() << "\n");
  ++NumUnreachableBlocks;

  std::mt19937_64 engine;
  std::uniform_int_distribution<int64_t> distribution;
  // Seed engine and create distribution
  if (!replaceSeed.empty()) {
    std::seed_seq seed(replaceSeed.begin(), replaceSeed.end());
    engine.seed(seed);
  } else {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine.seed(seed);
  }

  bool hasBeenModified = false;

  OpaquePredicate::clearUnreachable(block);
  hasBeenModified = true;

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
      DEBUG(errs() << "\t\tReplacing with new instruction: " << *newInst
                   << "\n");
      replacements.push_back(std::make_pair(&inst, (Instruction *)newInst));

    } else if (CmpInst *compare = dyn_cast<CmpInst>(&inst)) {
      DEBUG(errs() << "\t\tCompare Instruction\n");

      unsigned predicate = compare->getPredicate();
      unsigned newPredicate = predicate;

      if (compare->isFPPredicate()) {
        while (newPredicate == predicate) {
          unsigned choice = distribution(engine) % floatCompareSize;
          newPredicate = floatCompare[choice];
        }
      } else if (compare->isIntPredicate()) {
        while (newPredicate == predicate) {
          unsigned choice = distribution(engine) % intCompareSize;
          newPredicate = intCompare[choice];
        }
      } else {
        llvm_unreachable("Unknown comparison type");
      }
      CmpInst *newCompare =
          CmpInst::Create(compare->getOpcode(), newPredicate,
                          compare->getOperand(0), compare->getOperand(1));
      DEBUG(errs() << "\t\tReplacing with new instruction: " << *newCompare
                   << "\n");
      replacements.push_back(std::make_pair(&inst, (Instruction *)newCompare));

    } else if (isa<LoadInst>(&inst)) { // TODO
      DEBUG(errs() << "\t\tLoad Instruction\n");
      Type *type = inst.getType();
      if (!type->isFloatTy() && !type->isIntegerTy()) {
        DEBUG(errs()
              << "\t\tSkipping -- neither floating point nor integer type\n");
        continue;
      }
    } else if (isa<StoreInst>(&inst)) { // TODO
      DEBUG(errs() << "\t\tStore Instruction\n");
      Type *type = inst.getOperand(0)->getType();
      if (!type->isFloatTy() && !type->isIntegerTy()) {
        DEBUG(errs()
              << "\t\tSkipping -- neither floating point nor integer type\n");
        continue;
      }
    }
  }

  if (replacements.empty()) {
    // Do some warning
    errs() << "WARNING: Unable to find suitable replacements for unreachable "
              "block -- will be optimised away\n";
    ++NumUnviableBlocks;
  } else {
    for (auto &pair : replacements) {
      ReplaceInstWithInst(pair.first, pair.second);
      ++NumInstReplaced;
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
